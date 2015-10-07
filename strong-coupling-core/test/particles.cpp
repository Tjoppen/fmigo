#include "scQuat.h"
#include "scVec3.h"
#include "scSolver.h"
#include "scSlave.h"
#include "scRigidBody.h"
#include "scConnector.h"
#include "scConstraint.h"
#include "scLockConstraint.h"
#include "stdio.h"
#include <vector>
#include "string.h"

void printHelp(char * command){
    printf("\nUsage:\n");
    printf("\t%s [options]\n\n",command);
    printf("[options]:\n");
    printf("\t--numBodies N\tSets the number of bodies to N\n");
    printf("\t--help,-h\tPrint help and quit\n");
    printf("\n");
}

/*
 * Runs "delete" on all instances in a vector, and removes all of them from the vector.
 */
template<typename T>
void deleteVectorContent(std::vector<T> v){
    while(v.size() > 0){
        delete v.back();
        v.pop_back();
    }
}

/*
 * Prints out the first row of CSV with variable names, for example "time,x0,f0,x1,f1,..." depending on the number of bodies.
 */
void printFirstCSVRow(std::vector<scRigidBody*> bodies){
    printf("time");
    for (int i = 0; i < bodies.size(); ++i){
        printf(",x%d", i);
        printf(",f%d", i);
    }
    printf("\n");
}

/*
 * Prints a CSV data row
 */
void printCSVRow(double t, std::vector<scRigidBody*> bodies){
    // Print results
    printf("%lf", t);
    for (int j = 0; j < bodies.size(); ++j){
        scRigidBody * body = bodies[j];
        printf(",%lf", body->m_position[0]);
        printf(",%lf", body->m_force[0]);
    }
    printf("\n");
}

int main(int argc, char ** argv){
    int N = 2,
        NT = 10,
        quiet = 0;
    double  dt = 0.01,
            relaxation = 3,
            compliance = 0.001,
            invMass = 1,
            invInertia = 1;

    // Parse arguments
    for (int i = 0; i < argc; ++i){
        char * a = argv[i];
        int last = (i == argc-1);

        if(!last){
            if(!strcmp(a,"--numBodies"))   N = atoi(argv[i+1]);
            if(!strcmp(a,"--numSteps"))    NT = atoi(argv[i+1]);
            if(!strcmp(a,"--compliance"))  compliance = atof(argv[i+1]);
            if(!strcmp(a,"--relaxation"))  relaxation = atof(argv[i+1]);
            if(!strcmp(a,"--timeStep"))    dt = atof(argv[i+1]);
        }

        if(strcmp(argv[i],"--help")==0 || strcmp(argv[i],"-h")==0){
            printHelp(argv[0]);
            return 0;
        }

        if(strcmp(argv[i],"--quiet")==0 || strcmp(argv[i],"-q")==0)
            quiet = 1;
    }

    scSolver solver;

    // Init bodies / slave systems
    std::vector<scRigidBody*> bodies;
    std::vector<scSlave*> slaves;
    std::vector<scConnector*> connectors;
    std::vector<scConstraint*> constraints;
    scConnector * lastConnector = NULL;
    for (int i = 0; i < N; ++i){

        // Create body
        scRigidBody * body = new scRigidBody();
        body->m_position[0] = (double)i;
        body->m_invMass = i==0 ? 0 : invMass;
        body->m_invInertia = i==0 ? 0 : invInertia;

        // Create slave
        scSlave * slave = new scSlave();

        // Create connector at center of mass of the body
        scConnector * conn = new scConnector();
        slave->addConnector(conn);

        // Note: Must add slave *after* adding connectors
        solver.addSlave(slave);

        // Create lock joint between this and last connector
        if(lastConnector != NULL){
            scConstraint * constraint = new scLockConstraint(lastConnector, conn);
            solver.addConstraint(constraint);
            constraints.push_back(constraint);
        }

        lastConnector = conn;

        bodies.push_back(body);
        slaves.push_back(slave);
        connectors.push_back(conn);
    }

    // Set spook parameters on all equations
    solver.setSpookParams(relaxation,compliance,dt);

    // Get system equations
    std::vector<scEquation *> eqs;
    solver.getEquations(&eqs);

    // Print CSV first column
    if(!quiet) printFirstCSVRow(bodies);
    if(!quiet) printCSVRow(0,bodies);

    // Time loop
    for (int i = 0; i < NT; ++i){

        // Simulation time
        double t = i*dt;

        for (int j = 0; j < N; ++j){
            scRigidBody * body = bodies[j];
            scConnector * conn = connectors[j];

            // Set connector values
            scVec3::copy(conn->m_position,body->m_position);
            scVec3::copy(conn->m_velocity,body->m_velocity);
        }

        for (int j = 0; j < eqs.size(); ++j){
            scEquation * eq = eqs[j];
            double seed[3];

            // Set jacobians
            eq->getSpatialJacobianSeedA(seed);
            scVec3::scale(seed,invMass);
            eq->setSpatialJacobianA(seed);

            eq->getRotationalJacobianSeedA(seed);
            scVec3::scale(seed,invInertia);
            eq->setRotationalJacobianA(seed);

            eq->getSpatialJacobianSeedB(seed);
            scVec3::scale(seed,invMass);
            eq->setSpatialJacobianB(seed);

            eq->getRotationalJacobianSeedB(seed);
            scVec3::scale(seed,invInertia);
            eq->setRotationalJacobianB(seed);
        }

        // Solve system
        solver.solve();

        // Add constraint forces to the bodies
        for (int j = 0; j < slaves.size(); ++j){
            /*for (int k = 0; k < 3; ++k){
                printf("f%d[%d] = %f (c_index=%d)\n",j,k,slaves[j]->getConnector(0)->m_force[k],slaves[j]->getConnector(0)->m_index);
            }*/
            scVec3::copy( bodies[j]->m_force  , slaves[j]->getConnector(0)->m_force  );
            scVec3::copy( bodies[j]->m_torque , slaves[j]->getConnector(0)->m_torque );
        }

        // Integrate bodies
        for (int j = 0; j < N; ++j){
            scRigidBody * body = bodies[j];
            body->integrate(dt);
        }

        // Print results
        if(!quiet) printCSVRow(t,bodies);
    }

    deleteVectorContent(bodies);
    deleteVectorContent(slaves);
    deleteVectorContent(connectors);
    deleteVectorContent(constraints);
}
