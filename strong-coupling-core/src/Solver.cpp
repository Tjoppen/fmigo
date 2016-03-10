#include "sc/Solver.h"
#include "sc/Slave.h"
#include "sc/Connector.h"
#include "sc/Vec3.h"
#include "stdlib.h"
#include "stdio.h"
#include <algorithm>

extern "C" {
#include "umfpack.h"
}

using namespace sc;
using namespace std;

Solver::Solver(){
    m_connectorIndexCounter = 0;
    equations_dirty = true;
}

Solver::~Solver(){}

void Solver::addSlave(Slave * slave){
    equations_dirty = true;
    m_slaves.push_back(slave);
    int n = slave->numConnectors();
    for (int i = 0; i < n; ++i){
        slave->getConnector(i)->m_index = m_connectorIndexCounter++;
        m_connectors.push_back(slave->getConnector(i));
    }
}

void Solver::addConstraint(Constraint * constraint){
    equations_dirty = true;
    m_constraints.push_back(constraint);
}

int Solver::getSystemMatrixRows(){
  if (equations_dirty) {
    // Easy. Just count total number of equations
    int i,
        nConstraints=m_constraints.size(),
        nEquations=0;
    for(i=0; i<nConstraints; ++i){
        nEquations += m_constraints[i]->getNumEquations();
    }

    numsystemrows = nEquations;
  }
  return numsystemrows;
}

int Solver::getSystemMatrixCols(){
    int numSlaves = m_slaves.size(),
        numConnectors = 0,
        i;
    for(i=0; i<numSlaves; ++i){
        numConnectors += m_slaves[i]->numConnectors();
    }
    // Each connector has 6 dofs
    return 6*numConnectors;
}

vector<Equation*> Solver::getEquations(){
    if (equations_dirty) {
        eqs.clear();
    }

    int ofs = 0;
    for (int i=0; i< m_constraints.size(); ++i){
        int nEquations = m_constraints[i]->getNumEquations();
        for (int j=0; j<nEquations; ++j, ofs++){
            Equation *eq = m_constraints[i]->getEquation(j);
            eq->m_index = ofs;

            if (equations_dirty) {
                eqs.push_back(eq);
            } else {
                eqs[ofs] = eq;
            }
        }
    }

    return eqs;
}

void Solver::setSpookParams(double relaxation, double compliance, double timeStep){
    m_a = 4/(1+4*relaxation)/timeStep;
    m_b = 1/(1+4*relaxation);
    m_epsilon = 4 * compliance / (timeStep*timeStep * (1 + 4*relaxation));
    m_timeStep = timeStep;
}

void Solver::resetConstraintForces(){
    for (int i = 0; i < m_connectors.size(); ++i){
        Connector * c = m_connectors[i];
        c->m_force.set(0,0,0);
        c->m_torque.set(0,0,0);
    }
}

void Solver::updateConstraints(){
    for(int i=0; i<m_constraints.size(); i++){
        m_constraints[i]->update();
    }
}

void Solver::constructS() {
    Srow.clear();
    Scol.clear();
    Sval.clear();

    int neq = eqs.size();
    for (int i = 0; i < neq; ++i){
        for (int j = 0; j < neq; ++j){
            // We are at element i,j in S
            Equation * ei = eqs[i];
            Equation * ej = eqs[j];

            //check for overlap in connector FMUs, not the connectors themselves
            if (ei->haveOverlappingFMUs(ej)) {
                Srow.push_back(i);
                Scol.push_back(j);
                Sval.push_back(0);  //dummy value
            }
        }
    }

    //remember how many entries we have that change every time step
    nchangingentries = Srow.size();

    // Add regularization to diagonal entries
    for (int i = 0; i < eqs.size(); ++i){
        if(m_epsilon > 0){

            int found = 0;

            // Find the corresponding triplet
            for(int j = 0; j < Srow.size(); ++j){
                if(Srow[j] == i && Scol[j] == i){
                    Sval[j] += m_epsilon;
                    found = 1;
                    break;
                }
            }

            // Could not find triplet. Add it.
            if(!found){
                Sval.push_back(m_epsilon);
                Srow.push_back(i);
                Scol.push_back(i);
            }
        }
    }
}

void Solver::prepare() {
    getSystemMatrixRows();
    getEquations();
    constructS();
    equations_dirty = false;
}

void Solver::solve(bool holonomic){
    solve(holonomic, 0);
}

void Solver::solve(bool holonomic, int printDebugInfo){
    int i, j, k, l;
    getEquations();
    int numRows = getSystemMatrixRows(),
        neq = eqs.size();

    // Compute RHS
    rhs.reserve(numRows);
    for(i=0; i<neq; ++i){
        Equation * eq = eqs[i];
        double  Z = eq->getFutureVelocity() - eq->getVelocity(),
                GW = eq->getVelocity(),
                g = eq->getViolation();
        if (holonomic) {
            rhs[i] = -m_a * g - m_b * GW - Z; // RHS = -a*g -b*G*W -Z
        } else {
            rhs[i] =          - m_b * GW - Z; //nonholonomic
        }
    }

    // Compute matrix S = G * inv(M) * G' = G * z
    // Should be easy, since we already got the entries from the user
    //TODO: figure out if these vary, reset each time
    if (equations_dirty) {
        //NOTE: this will be slow
        constructS();
        //alright, we have the structure of the matrix - don't redo all this work unless we have to
        equations_dirty = false;
    }

    for (size_t x = 0; x < nchangingentries; x++) {
        int i = Srow[x];
        int j = Scol[x];
        // We are at element i,j in S
        Equation * ei = eqs[i];
        Equation * ej = eqs[j];

        //each S_ij = G_i*J_i^T
        //our job is to figure out J_i^T
        double val = 0;
        for (Connector *conn : ei->getConnectors()) {
            std::pair<int,int> key(conn->m_index, ej->m_index);
            if (m_mobilities.find(key) != m_mobilities.end()) {
                val += ei->jacobianElementForConnector(conn).multiply(m_mobilities[key]);
            }
        }

        if (i == j) {
            val += m_epsilon;
        }

        Sval[x] = val;
    }

    // Print matrices
    if(printDebugInfo){
        for (int i = 0; i < Srow.size(); ++i){
            fprintf(stderr, "(%d,%d) => %g\n",Scol[i],Srow[i],Sval[i]);
        }
        char empty = '0';
        char tab = '\t';

        fprintf(stderr, "E = [\n");
        for (int i = 0; i < eqs.size(); ++i){ // Rows
            for (int j = 0; j < eqs.size(); ++j){ // Cols
                if(i==j)
                    fprintf(stderr, "%g\t", m_epsilon);
                else
                    fprintf(stderr, "0\t");
            }
            if(i == eqs.size()-1)
                fprintf(stderr, "]\n");
            else
                fprintf(stderr, ";\n");
        }

        fprintf(stderr, "S = [\n");
        for (int i = 0; i < eqs.size(); ++i){ // Rows
            for (int j = 0; j < eqs.size(); ++j){ // Cols

                // Find element i,j
                int found = 0;
                for(int k = 0; k < Srow.size(); ++k){
                    if(Srow[k] == i && Scol[k] == j){
                        fprintf(stderr, "%g\t", Sval[k]);
                        found = 1;
                        break;
                    }
                }
                if(!found)
                    fprintf(stderr, "%c\t",empty);
            }
            if(i == eqs.size()-1)
                fprintf(stderr, "]\n");
            else
                fprintf(stderr, ";\n");
        }
    }

    // convert vectors to arrays
    aSrow.reserve(Srow.size()+1);
    aScol.reserve(Scol.size()+1);
    aSval.reserve(Sval.size()+1);
    for (int i = 0; i < Srow.size(); ++i){
        aSval[i] = Sval[i];
        aScol[i] = Scol[i];
        aSrow[i] = Srow[i];

        //printf("(%d,%d) = %g\n", Srow[i], Scol[i], Sval[i]);
    }

    void *Symbolic, *Numeric;
    double Info [UMFPACK_INFO], Control [UMFPACK_CONTROL];

    // Default control
    umfpack_di_defaults (Control) ;

    // convert to column form
    int nz = Sval.size(),       // Non-zeros
        n = eqs.size(),         // Number of equations
        nz1 = std::max(nz,1) ;  // ensure arrays are not of size zero.
    Ap.reserve(n+1);
    Ai.reserve(nz1);
    lambda.reserve(n);
    Ax.reserve(nz1);

    if(printDebugInfo)
        printf("n=%d, nz=%d\n",n, nz);

    // Triplet form to column form
    int status = umfpack_di_triplet_to_col (n, n, nz, aSrow.data(), aScol.data(), aSval.data(), Ap.data(), Ai.data(), Ax.data(), (int *) NULL) ;
    if (status < 0){
        umfpack_di_report_status (Control, status) ;
        fprintf(stderr, "umfpack_di_triplet_to_col failed\n") ;
        exit(1);
    }

    // symbolic factorization
    status = umfpack_di_symbolic (n, n, Ap.data(), Ai.data(), Ax.data(), &Symbolic, Control, Info) ;
    if (status < 0){
        umfpack_di_report_info (Control, Info) ;
        umfpack_di_report_status (Control, status) ;
        fprintf(stderr,"umfpack_di_symbolic failed\n") ;
        exit(1);
    }

    // numeric factorization
    status = umfpack_di_numeric (Ap.data(), Ai.data(), Ax.data(), Symbolic, &Numeric, Control, Info) ;
    if (status < 0){
        umfpack_di_report_info (Control, Info) ;
        umfpack_di_report_status (Control, status) ;
        fprintf(stderr,"umfpack_di_numeric failed\n") ;
        exit(1);
    }

    // solve S*lambda = B
    status = umfpack_di_solve (UMFPACK_A, Ap.data(), Ai.data(), Ax.data(), lambda.data(), rhs.data(), Numeric, Control, Info) ;
    umfpack_di_report_info (Control, Info) ;
    umfpack_di_report_status (Control, status) ;
    if (status < 0){
        fprintf(stderr,"umfpack_di_solve failed\n") ;
        exit(1);
    }

    // Set current connector forces to zero
    resetConstraintForces();

    // Store results
    // Remember that we need to divide lambda by the timestep size
    // f = G'*lambda
    for (int i = 0; i<eqs.size(); ++i){
        Equation * eq = eqs[i];
        double l = lambda[i] / m_timeStep;

        for (Connector *conn : eq->getConnectors()) {
            JacobianElement G = eq->jacobianElementForConnector(conn);
            Vec3 f = G.getSpatial()    * l;
            Vec3 t = G.getRotational() * l;
            conn->m_force  += f;
            conn->m_torque += t;
        }
    }

#if 0
    // Print matrices
    if(printDebugInfo){

        printf("RHS = [\n");
        for (int i = 0; i < eqs.size(); ++i){
            printf("%g\n",rhs[i]);
        }
        printf("]\n");

        printf("umfpackSolution = [\n");
        for (int i = 0; i < eqs.size(); ++i){
            printf("%g\n",lambda[i]);
        }
        printf("]\n");

        printf("octaveSolution = S \\ RHS\n");


        printf("Gt_lambda = [\n");
        int numSlaves = m_slaves.size();
        for(int i=0; i<numSlaves; ++i){
            int Nconns = m_slaves[i]->numConnectors();
            for(int j=0; j<Nconns; ++j){
                Connector * c = m_slaves[i]->getConnector(j);
                printf("%g\n%g\n%g\n%g\n%g\n%g\n",
                    c->m_force[0],
                    c->m_force[1],
                    c->m_force[2],
                    c->m_torque[0],
                    c->m_torque[1],
                    c->m_torque[2]);
            }
        }
        printf("]\n");
    }
#endif

    umfpack_di_free_symbolic(&Symbolic);
    umfpack_di_free_numeric(&Numeric);
}

/// Get a constraint
Constraint * Solver::getConstraint(int i){
    return m_constraints[i];
}

/// Get a constraint
int Solver::getNumConstraints(){
    return m_constraints.size();
}
