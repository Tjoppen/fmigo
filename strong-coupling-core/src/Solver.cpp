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

void Solver::getEquations(std::vector<Equation*> * result){
    int i,j,
        nConstraints=m_constraints.size();
    for(i=0; i<nConstraints; ++i){
        int nEquations = m_constraints[i]->getNumEquations();
        for(j=0; j<nEquations; ++j){
            result->push_back(m_constraints[i]->getEquation(j));
        }
    }
}

void Solver::getEquationsFast(){
    if (equations_dirty) {
        eqs.clear();
    }

    int ofs = 0;
    for (int i=0; i< m_constraints.size(); ++i){
        int nEquations = m_constraints[i]->getNumEquations();
        for (int j=0; j<nEquations; ++j, ofs++){
            Equation *eq = m_constraints[i]->getEquation(j);

            if (equations_dirty) {
                eqs.push_back(eq);
            } else {
                eqs[ofs] = eq;
            }
        }
    }
}

void Solver::setSpookParams(double relaxation, double compliance, double timeStep){
    int i,j, nConstraints=m_constraints.size();
    for(i=0; i<nConstraints; ++i){
        int nEquations = m_constraints[i]->getNumEquations();
        for(j=0; j<nEquations; ++j){
            m_constraints[i]->getEquation(j)->setSpookParams(relaxation, compliance, timeStep);
        }
    }
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

            if     (ei->getConnA() == ej->getConnA() ||
                    ei->getConnA() == ej->getConnB() ||
                    ei->getConnB() == ej->getConnA() ||
                    ei->getConnB() == ej->getConnB()) {
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
        double eps = eqs[i]->m_epsilon;
        if(eps > 0){

            int found = 0;

            // Find the corresponding triplet
            for(int j = 0; j < Srow.size(); ++j){
                if(Srow[j] == i && Scol[j] == i){
                    Sval[j] += eps;
                    found = 1;
                    break;
                }
            }

            // Could not find triplet. Add it.
            if(!found){
                Sval.push_back(eps);
                Srow.push_back(i);
                Scol.push_back(i);
            }
        }
    }
}

void Solver::prepare() {
    getSystemMatrixRows();
    getEquationsFast();
    constructS();
    equations_dirty = false;
}

void Solver::solve(){
    solve(0);
}

void Solver::solve(int printDebugInfo){
    int i, j, k, l;
    getEquationsFast();
    int numRows = getSystemMatrixRows(),
        neq = eqs.size();

    // Compute RHS
    rhs.reserve(numRows);
    for(i=0; i<neq; ++i){
        Equation * eq = eqs[i];
        double  Z = eq->getFutureVelocity() - eq->getVelocity(),
                GW = eq->getVelocity(),
                g = eq->getViolation(),
                a = eq->m_a,
                b = eq->m_b;
        rhs[i] = -a * g  - b * GW  - Z; // RHS = -a*g -b*G*W -Z
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

        double val = 0;
        if(ei->getConnA() == ej->getConnA()){
            val += ei->getGA().multiply(ej->getddA());
        }
        if(ei->getConnA() == ej->getConnB()){
            val += ei->getGA().multiply(ej->getddB());
        }
        if(ei->getConnB() == ej->getConnA()){
            val += ei->getGB().multiply(ej->getddA());
        }
        if(ei->getConnB() == ej->getConnB()){
            val += ei->getGB().multiply(ej->getddB());
        }

        if (i == j) {
            if (ei->m_epsilon != ej->m_epsilon) {
                fprintf(stderr, "epsilons differ\n");
                exit(1);
            }
            val += ei->m_epsilon;
        }

        Sval[x] = val;
    }

#if 0
    // Print matrices
    if(printDebugInfo){
        for (int i = 0; i < Srow.size(); ++i){
            printf("(%d,%d) => %g\n",Scol[i],Srow[i],Sval[i]);
        }
        char empty = '0';
        char tab = '\t';
        printf("G = [\n");
        for(int i=0; i<eqs.size(); ++i){
            Equation * eq = eqs[i];
            Connector * connA = eq->getConnA();
            Connector * connB = eq->getConnB();

            //printf("%d %d\n",connA->m_index,connB->m_index);

            int swapped = 0;
            if(connA->m_index > connB->m_index){
                Connector * temp = connA;
                connA = connB;
                connB = temp;
                swapped = 1;
            }

            // Print empty until first
            for (int j = 0; j < 6*connA->m_index; ++j){
                printf("%c\t",empty);
            }

            // Print contents of first ( 6 jacobian entries )
            JacobianElement G = !swapped ? eq->getGA() : eq->getGB();
            printf("%g%c%g%c%g%c%g%c%g%c%g%c",
                G.getSpatial().x(),tab,
                G.getSpatial().y(),tab,
                G.getSpatial().z(),tab,
                G.getRotational().x(),tab,
                G.getRotational().y(),tab,
                G.getRotational().z(),tab);

            // Print empty until second
            for (int j = 6*(connA->m_index+1); j < 6*connB->m_index; ++j){
                printf("%c\t",empty);
            }

            // Print contents of second ( 6 jacobian entries )
            JacobianElement G2 = !swapped ? eq->getGB() : eq->getGA();
            printf("%g%c%g%c%g%c%g%c%g%c%g%c",
                G2.getSpatial().x(),tab,
                G2.getSpatial().y(),tab,
                G2.getSpatial().z(),tab,
                G2.getRotational().x(),tab,
                G2.getRotational().y(),tab,
                G2.getRotational().z(),tab);

            // Print empty until end of row
            for (int j = 6*(connB->m_index+1); j < getSystemMatrixCols(); ++j){
                printf("%c\t",empty);
            }

            if(i == eqs.size()-1)
                printf("]\n");
            else
                printf(";\n");
        }

        printf("D = [\n");
        for(int i=0; i<eqs.size(); ++i){
            Equation * eq = eqs[i];
            Connector * connA = eq->getConnA();
            Connector * connB = eq->getConnB();

            //printf("%d %d\n",connA->m_index,connB->m_index);

            int swapped = 0;
            if(connA->m_index > connB->m_index){
                Connector * temp = connA;
                connA = connB;
                connB = temp;
                swapped = 1;
            }

            // Print empty until first
            for (int j = 0; j < 6*connA->m_index; ++j){
                printf("%c\t",empty);
            }

            // Print contents of first ( 6 jacobian entries )
            JacobianElement G = !swapped ? eq->getddA() : eq->getddB();
            printf("%g%c%g%c%g%c%g%c%g%c%g%c",
                G.getSpatial().x(),tab,
                G.getSpatial().y(),tab,
                G.getSpatial().z(),tab,
                G.getRotational().x(),tab,
                G.getRotational().y(),tab,
                G.getRotational().z(),tab);

            // Print empty until second
            for (int j = 6*(connA->m_index+1); j < 6*connB->m_index; ++j){
                printf("%c\t",empty);
            }

            // Print contents of second ( 6 jacobian entries )
            JacobianElement G2 = !swapped ? eq->getddB() : eq->getddA();
            printf("%g%c%g%c%g%c%g%c%g%c%g%c",
                G2.getSpatial().x(),tab,
                G2.getSpatial().y(),tab,
                G2.getSpatial().z(),tab,
                G2.getRotational().x(),tab,
                G2.getRotational().y(),tab,
                G2.getRotational().z(),tab);

            // Print empty until end of row
            for (int j = 6*(connB->m_index+1); j < getSystemMatrixCols(); ++j){
                printf("%c\t",empty);
            }

            if(i == eqs.size()-1)
                printf("]\n");
            else
                printf(";\n");
        }

        printf("E = [\n");
        for (int i = 0; i < eqs.size(); ++i){ // Rows
            for (int j = 0; j < eqs.size(); ++j){ // Cols
                if(i==j)
                    printf("%g\t", eqs[i]->m_epsilon);
                else
                    printf("0\t");
            }
            if(i == eqs.size()-1)
                printf("]\n");
            else
                printf(";\n");
        }

        printf("S = [\n");
        for (int i = 0; i < eqs.size(); ++i){ // Rows
            for (int j = 0; j < eqs.size(); ++j){ // Cols

                // Find element i,j
                int found = 0;
                for(int k = 0; k < Srow.size(); ++k){
                    if(Srow[k] == i && Scol[k] == j){
                        printf("%g\t", Sval[k]);
                        found = 1;
                        break;
                    }
                }
                if(!found)
                    printf("%c\t",empty);
            }
            if(i == eqs.size()-1)
                printf("]\n");
            else
                printf(";\n");
        }
    }
#endif

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
        double l = lambda[i] / eq->m_timeStep;

        JacobianElement GA = eq->getGA();
        JacobianElement GB = eq->getGB();
        Vec3 fA = GA.getSpatial()    * l;
        Vec3 tA = GA.getRotational() * l;
        Vec3 fB = GB.getSpatial()    * l;
        Vec3 tB = GB.getRotational() * l;

        // We are on row i in the matrix
        eq->getConnA()->m_force  += fA;
        eq->getConnA()->m_torque += tA;
        eq->getConnB()->m_force  += fB;
        eq->getConnB()->m_torque += tB;

        /*
        printf("forceA  += %g %g %g\n", fA[0], fA[1], fA[2]);
        printf("torqueA += %g %g %g\n", tA[0], tA[1], tA[2]);
        printf("forceB  += %g %g %g\n", fB[0], fB[1], fB[2]);
        printf("torqueB += %g %g %g\n", tB[0], tB[1], tB[2]);


        printf("   GAs = %g %g %g\n", GA.getSpatial()[0], GA.getSpatial()[1], GA.getSpatial()[2]);
        printf("   GBs = %g %g %g\n", GB.getSpatial()[0], GB.getSpatial()[1], GB.getSpatial()[2]);
        printf("   GAr = %g %g %g\n", GA.getRotational()[0], GA.getRotational()[1], GA.getRotational()[2]);
        printf("   GBr = %g %g %g\n", GB.getRotational()[0], GB.getRotational()[1], GB.getRotational()[2]);

        printf("\n");
        */
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
