#include "scSolver.h"
#include "scSlave.h"
#include "scConnector.h"
#include "scVec3.h"
#include "stdlib.h"
#include "stdio.h"

extern "C" {
#include "umfpack.h"
}

scSolver::scSolver(){
    m_connectorIndexCounter = 0;
}

scSolver::~scSolver(){

}

void scSolver::addSlave(scSlave * slave){
    m_slaves.push_back(slave);
    int n = slave->numConnectors();
    for (int i = 0; i < n; ++i){
        slave->getConnector(i)->m_index = m_connectorIndexCounter++;
        m_connectors.push_back(slave->getConnector(i));
    }
}

void scSolver::addConstraint(scConstraint * constraint){
    m_constraints.push_back(constraint);
}

int scSolver::getSystemMatrixRows(){
    // Easy. Just count total number of equations
    int i,
        nConstraints=m_constraints.size(),
        nEquations=0;
    for(i=0; i<nConstraints; ++i){
        nEquations += m_constraints[i]->getNumEquations();
    }
    return nEquations;
}

int scSolver::getSystemMatrixCols(){
    int numSlaves = m_slaves.size(),
        numConnectors = 0,
        i;
    for(i=0; i<numSlaves; ++i){
        numConnectors += m_slaves[i]->numConnectors();
    }
    // Each connector has 6 dofs
    return 6*numConnectors;
}

void scSolver::getEquations(std::vector<scEquation*> * result){
    int i,j,
        nConstraints=m_constraints.size();
    for(i=0; i<nConstraints; ++i){
        int nEquations = m_constraints[i]->getNumEquations();
        for(j=0; j<nEquations; ++j){
            result->push_back(m_constraints[i]->getEquation(j));
        }
    }
}

void scSolver::setSpookParams(double relaxation, double compliance, double timeStep){
    int i,j, nConstraints=m_constraints.size();
    for(i=0; i<nConstraints; ++i){
        int nEquations = m_constraints[i]->getNumEquations();
        for(j=0; j<nEquations; ++j){
            m_constraints[i]->getEquation(j)->setSpookParams(relaxation, compliance, timeStep);
        }
    }
}

void scSolver::resetConstraintForces(){
    for (int i = 0; i < m_connectors.size(); ++i){
        scConnector * c = m_connectors[i];
        scVec3::set(c->m_force,0,0,0);
        scVec3::set(c->m_torque,0,0,0);
    }
}

void scSolver::solve(){

    /*
    Need to compute
          lambda = (S + E) \ ( - a*g - b*G*W - Z );
     where
          S = G1 * inv(M1) * transpose(G1) + G2 * inv(M2) * transpose(G2) + ...
    */

    int i, j, k, l;
    std::vector<scEquation*> eqs;
    getEquations(&eqs);
    int numRows = getSystemMatrixRows(), neq = eqs.size(), nconstraints=m_constraints.size();
    double * rhs = (double*)malloc(numRows*sizeof(double));

    // Compute RHS
    for(i=0; i<neq; ++i){
        scEquation * eq = eqs[i];
        //printf("v=%f\n", eq->getVelocity());
        rhs[i] = eq->m_a * eq->getViolation() + eq->m_b * eq->getVelocity(); // Z?
    }

    // Compute matrix S+E
    // Should be easy, since we already got the entries from the user
    // If these were sparse, we could easily skip the zeros
    std::vector<int> Srow;
    std::vector<int> Scol;
    std::vector<double> Sval;
    for (i = 0; i < nconstraints; ++i){ // Loop over all 6x6 blocks in S
        scConstraint * c0 = m_constraints[i];

        for (j = 0; j < nconstraints; ++j){
            scConstraint * c1 = m_constraints[j];

            int block_row = i;
            int block_col = j;

            if(i == j){
                // Diagonal block, simple

                int neq = c1->getNumEquations();
                for (k = 0; k < neq; ++k){
                    scEquation * eqA = c0->getEquation(k);
                    for (l = 0; l < neq; ++l){
                        scEquation * eqB = c1->getEquation(l);
                        int row = block_row * 6 + l;
                        int col = block_col * 6 + k;

                        // TODO the jacobian should be in each connector
                        double val = eqA->GmultG(eqB->m_invMGt);

                        Srow.push_back(row);
                        Scol.push_back(col);
                        Sval.push_back(val);

                        //printf("(%d,%d) = %f\n", row, col, val);
                    }
                }

            } else {
                // Off diagonal block

                int neqA = c0->getNumEquations();
                int neqB = c1->getNumEquations();

                for (k = 0; k < neqA; ++k){
                    scEquation * eqA = c0->getEquation(k);

                    for (l = 0; l < neqB; ++l){
                        scEquation * eqB = c1->getEquation(l);
                        int row = block_row * 6 + l;
                        int col = block_col * 6 + k;
                        double val = 0;
                        if(c0->m_connA == c1->m_connA){
                            val +=  eqA->m_G[0] * eqB->m_invMGt[0] +
                                    eqA->m_G[1] * eqB->m_invMGt[1] +
                                    eqA->m_G[2] * eqB->m_invMGt[2] +
                                    eqA->m_G[3] * eqB->m_invMGt[3] +
                                    eqA->m_G[4] * eqB->m_invMGt[4] +
                                    eqA->m_G[5] * eqB->m_invMGt[5];
                        }
                        if(c0->m_connA == c1->m_connB){
                            val +=  eqA->m_G[0] * eqB->m_invMGt[6] +
                                    eqA->m_G[1] * eqB->m_invMGt[7] +
                                    eqA->m_G[2] * eqB->m_invMGt[8] +
                                    eqA->m_G[3] * eqB->m_invMGt[9] +
                                    eqA->m_G[4] * eqB->m_invMGt[10] +
                                    eqA->m_G[5] * eqB->m_invMGt[11];
                        }
                        if(c0->m_connB == c1->m_connA){
                            val +=  eqA->m_G[6] * eqB->m_invMGt[0] +
                                    eqA->m_G[7] * eqB->m_invMGt[1] +
                                    eqA->m_G[8] * eqB->m_invMGt[2] +
                                    eqA->m_G[9] * eqB->m_invMGt[3] +
                                    eqA->m_G[10] * eqB->m_invMGt[4] +
                                    eqA->m_G[11] * eqB->m_invMGt[5];
                        }
                        if(c0->m_connB == c1->m_connB){
                            val +=  eqA->m_G[6] * eqB->m_invMGt[6] +
                                    eqA->m_G[7] * eqB->m_invMGt[7] +
                                    eqA->m_G[8] * eqB->m_invMGt[8] +
                                    eqA->m_G[9] * eqB->m_invMGt[9] +
                                    eqA->m_G[10] * eqB->m_invMGt[10] +
                                    eqA->m_G[11] * eqB->m_invMGt[11];
                        }
                        Sval.push_back(val);
                        Srow.push_back(row);
                        Scol.push_back(col);

                        //printf("OFF - DIAGONAL %d %d %f\n", row, col, val);
                    }
                }
            }
        }
    }

    // Add regularization to diagonal entries
    for (int i = 0; i < eqs.size(); ++i){
        double eps = eqs[i]->m_epsilon;
        if(eps > 0){
            for(int j = 0; j < Srow.size(); ++j){
                if(Srow[j] == i && Scol[j] == i){
                    Sval[j] += eps;
                    break;
                }
            }
        }
    }

    // convert vectors to arrays
    int * aSrow =    (int *)    malloc ((Srow.size()+1) * sizeof (int));
    int * aScol =    (int *)    malloc ((Scol.size()+1) * sizeof (int));
    double * aSval = (double *) malloc ((Sval.size()+1) * sizeof (double));
    for (int i = 0; i < Srow.size(); ++i){
        aSval[i] = Sval[i];
        aScol[i] = Scol[i];
        aSrow[i] = Srow[i];
    }

    void *Symbolic, *Numeric;
    double Info [UMFPACK_INFO], Control [UMFPACK_CONTROL];

    // Default control
    umfpack_di_defaults (Control) ;

    // convert to column form
    int nz = Sval.size(); // Non-zeros
    int n = eqs.size(); // Number of equations
    int nz1 = std::max(nz,1) ;  // ensure arrays are not of size zero.
    int * Ap = (int *) malloc ((n+1) * sizeof (int)) ;
    int * Ai = (int *) malloc (nz1 * sizeof (int)) ;
    double * lambda = (double *) malloc (n * sizeof (double)) ;
    double * Ax = (double *) malloc (nz1 * sizeof (double)) ;
    if (!Ap || !Ai || !Ax){
        fprintf(stderr, "out of memory\n") ;
    }

    // Triplet form to column form
    int status = umfpack_di_triplet_to_col (n, n, nz, aSrow, aScol, aSval, Ap, Ai, Ax, (int *) NULL) ;
    if (status < 0){
        umfpack_di_report_status (Control, status) ;
        fprintf(stderr, "umfpack_di_triplet_to_col failed\n") ;
    }

    // symbolic factorization
    status = umfpack_di_symbolic (n, n, Ap, Ai, Ax, &Symbolic, Control, Info) ;
    if (status < 0){
        umfpack_di_report_info (Control, Info) ;
        umfpack_di_report_status (Control, status) ;
        fprintf(stderr,"umfpack_di_symbolic failed\n") ;
    }

    // numeric factorization
    status = umfpack_di_numeric (Ap, Ai, Ax, Symbolic, &Numeric, Control, Info) ;
    if (status < 0){
        umfpack_di_report_info (Control, Info) ;
        umfpack_di_report_status (Control, status) ;
        fprintf(stderr,"umfpack_di_numeric failed\n") ;
    }

    // solve S*lambda = B
    status = umfpack_di_solve (UMFPACK_A, Ap, Ai, Ax, lambda, rhs, Numeric, Control, Info) ;
    umfpack_di_report_info (Control, Info) ;
    umfpack_di_report_status (Control, status) ;
    if (status < 0){
        fprintf(stderr,"umfpack_di_solve failed\n") ;
    }

    // Set current connector forces to zero
    resetConstraintForces();

    // Store results
    // Remember that we need to divide lambda by the timestep size
    // f = G'*lambda
    for (int i = 0; i<eqs.size(); ++i){
        scEquation * eq = eqs[i];
        double l = lambda[i] / eq->m_timeStep;
        double * G = eq->m_G;
        double fA[3] = { l*G[0], l*G[1],  l*G[2] };
        double tA[3] = { l*G[0], l*G[1],  l*G[2] };
        double fB[3] = { l*G[0], l*G[1],  l*G[2] };
        double tB[3] = { l*G[0], l*G[1],  l*G[2] };

        //printf("Setting forces for index %d and %d: %f %f %f\n", eq->getConnA()->m_index, eq->getConnB()->m_index,l*G[0], l*G[1],  l*G[2]);

        // We are on row i in the matrix
        scVec3::add(eq->getConnA()->m_force, eq->getConnA()->m_force,  fA);
        scVec3::add(eq->getConnA()->m_torque,eq->getConnA()->m_torque, tA);
        scVec3::add(eq->getConnB()->m_force, eq->getConnB()->m_force,  fB);
        scVec3::add(eq->getConnB()->m_torque,eq->getConnB()->m_torque, tB);
    }

    // Print matrices
    if(SCSOLVER_DEBUGPRINTS){

        char empty = '0';
        printf("G = [\n");
        for(int i=0; i<eqs.size(); ++i){
            scEquation * eq = eqs[i];
            scConnector * connA = eq->getConnA();
            scConnector * connB = eq->getConnB();

            //printf("%d %d\n",connA->m_index,connB->m_index);

            int swapped = 0;
            if(connA->m_index > connB->m_index){
                scConnector * temp = connA;
                connA = connB;
                connB = temp;
                swapped = 1;
            }

            // Print empty until first
            for (int j = 0; j < 6*connA->m_index; ++j){
                printf("%c\t",empty);
            }

            // Print contents of first ( 6 jacobian entries )
            for (int j = 0; j < 6; ++j){
                printf("%g\t", eq->m_G[j + swapped*6]);
            }

            // Print empty until second
            for (int j = 6*(connA->m_index+1); j < 6*connB->m_index; ++j){
                printf("%c\t",empty);
            }

            // Print contents of second ( 6 jacobian entries )
            for (int j = 0; j < 6; ++j){
                printf("%g\t", eq->m_G[6 + j - swapped*6]);
            }

            // Print empty until end of row
            for (int j = 6*(connB->m_index+1); j < getSystemMatrixCols(); ++j){
                printf("%c\t",empty);
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
    }

    free(rhs);
    free(lambda);
    free(aSrow);
    free(aScol);
    free(aSval);
    free(Ap);
    free(Ai);
    free(Ax);
    umfpack_di_free_symbolic(&Symbolic);
    umfpack_di_free_numeric(&Numeric);
}
