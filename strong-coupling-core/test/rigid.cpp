#include "RigidBody.h"
#include "sc/Quat.h"
#include "sc/Vec3.h"
#include "sc/Solver.h"
#include "sc/Slave.h"
#include "sc/Connector.h"
#include "sc/Constraint.h"
#include "sc/LockConstraint.h"
#include "sc/BallJointConstraint.h"
#include "sc/HingeMotorConstraint.h"
#include "sc/HingeConstraint.h"
#include <vector>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifdef SC_USE_OSG
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osgGA/TrackballManipulator>
#include <osgUtil/Version>
#include <osg/Version>
#endif

using namespace sc;

void printHelp(char * command){
    printf("\nUsage:\n\
\t%s [OPTIONS] [FLAGS]\
\n\
\n\
[OPTIONS]\n\
\n\
\t--compliance <number> \tGlobal constraint compliance.\n\
\t--gravityX   <number> \tGravity in X direction.\n\
\t--gravityY   <number> \tGravity in Y direction.\n\
\t--gravityZ   <number> \tGravity in Z direction.\n\
\t--numBodies  <integer>\tNumber of bodies.\n\
\t--numSteps   <integer>\tMax number of time steps. Infinite if not given.\n\
\t--relaxation <number> \tGlobal constraint relaxation.\n\
\t--timeStep   <number> \tTime step size.\n\
\n\
[FLAGS]\n\
\n\
\t--csv     \tPrint CSV data to STDOUT. Also triggers --quiet.\n\
\t--help,-h \tPrint help and quit.\n\
\t--quiet,-q\tDon't print stuff.\n\
\t--render  \tRender the scene using OSG.\n\n",command);
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
void printFirstCSVRow(std::vector<RigidBody*> bodies){
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
void printCSVRow(double t, std::vector<RigidBody*> bodies){
    // Print results
    printf("%lf", t);
    for (int j = 0; j < bodies.size(); ++j){
        RigidBody * body = bodies[j];
        printf(",%lf", body->m_position[0]);
        printf(",%lf", body->m_force[0]);
    }
    printf("\n");
}

int main(int argc, char ** argv){
    int N = 2,
        NT = INT_MAX,
        quiet = 0,
        debug = 0,
        render = 0,
        enableCSV = 0;
    double  dt = 0.01,
            relaxation = 3,
            compliance = 0.001,
            invMass = 1,
            gravityX = 0,
            gravityY = 0,
            gravityZ = 0;

    Vec3 invInertia(1,1,1);
    Vec3 halfExtents(0.1,0.05,0.05);

    // Parse arguments
    for (int i = 0; i < argc; ++i){
        char * a = argv[i];
        int last = (i == argc-1);

        // Flags with args
        if(!last){
            if(!strcmp(a,"--numBodies"))   N = atoi(argv[i+1]);
            if(!strcmp(a,"--numSteps"))    NT = atoi(argv[i+1]);
            if(!strcmp(a,"--compliance"))  compliance = atof(argv[i+1]);
            if(!strcmp(a,"--relaxation"))  relaxation = atof(argv[i+1]);
            if(!strcmp(a,"--timeStep"))    dt = atof(argv[i+1]);
            if(!strcmp(a,"--gravityX"))    gravityX = atof(argv[i+1]);
            if(!strcmp(a,"--gravityY"))    gravityY = atof(argv[i+1]);
            if(!strcmp(a,"--gravityZ"))    gravityZ = atof(argv[i+1]);
        }

        // Flags without args
        if(!strcmp(a,"--render"))   render = 1;
        if(!strcmp(a,"--debug"))    debug =  1;
        if(!strcmp(a,"--csv")){
            enableCSV = 1;
            quiet = 1;
        }

        if(strcmp(argv[i],"--help")==0 || strcmp(argv[i],"-h")==0){
            printHelp(argv[0]);
            return 0;
        }

        if(strcmp(argv[i],"--quiet")==0 || strcmp(argv[i],"-q")==0)
            quiet = 1;
    }

    Solver solver;

    // Init bodies / slave systems
    std::vector<RigidBody*> bodies;
    std::vector<Slave*> slaves;
    std::vector<Connector*> connectors;
    std::vector<Constraint*> constraints;
    Connector * lastConnector = NULL;
    for (int i = 0; i < N; ++i){

        // Create body
        RigidBody * body = new RigidBody();
        body->m_position[0] = (double)halfExtents[0]*2*(i-N/2);
        body->m_invMass = i==0 ? 0 : invMass;
        if(i==0){
            body->setLocalInertiaAsBox(0,halfExtents);
        } else {
            body->setLocalInertiaAsBox(1,halfExtents);
        }
        body->m_gravity.set(gravityX,gravityY,gravityZ);

        // Create slave
        Slave * slave = new Slave();

        // Create connector at center of mass of the body
        Connector * conn = new Connector();
        slave->addConnector(conn);
        conn->m_userData = (void*)body;

        // Note: Must add slave *after* adding connectors
        solver.addSlave(slave);

        // Create lock joint between this and last connector
        if(lastConnector != NULL){
            if(i%2==0){
                Constraint * constraint = new LockConstraint(   lastConnector, conn,
                                                                Vec3(halfExtents[0],0,0),
                                                                Vec3(-halfExtents[0],0,0),
                                                                Quat(0,0,0,1),
                                                                Quat(0,0,0,1));
                solver.addConstraint(constraint);
                constraints.push_back(constraint);
            } else {
                Constraint * constraint = new HingeConstraint(  lastConnector, conn,
                                                                Vec3( halfExtents[0],0,0),
                                                                Vec3(-halfExtents[0],0,0),
                                                                Vec3(0,0,1),
                                                                Vec3(0,0,1));
                solver.addConstraint(constraint);
                constraints.push_back(constraint);

            }
        }

        lastConnector = conn;

        bodies.push_back(body);
        slaves.push_back(slave);
        connectors.push_back(conn);
    }

    // Set spook parameters on all equations
    solver.setSpookParams(relaxation,compliance,dt);

    // Get system equations
    std::vector<Equation*> eqs = solver.getEquations();

    // Print CSV first column
    if(enableCSV) printFirstCSVRow(bodies);
    if(enableCSV) printCSVRow(0,bodies);

    #ifdef SC_USE_OSG
        osgViewer::Viewer * viewer;
        std::vector<osg::PositionAttitudeTransform *> transforms;

        if(render){

            if(!quiet) printf("OpenSceneGraph %s\n",osgUtilGetVersion());

            // Create viewer
            viewer = new osgViewer::Viewer();

            // Root group
            osg::Group* root = new osg::Group();

            // Create boxes for each body
            for(int i=0; i<bodies.size(); i++){
                osg::Geode* boxGeode = new osg::Geode();
                osg::Box* box = new osg::Box(
                    osg::Vec3(0,0,0),
                    2*halfExtents[0],
                    2*halfExtents[1],
                    2*halfExtents[2]
                );
                box->setDataVariance(osg::Object::DYNAMIC);
                osg::ShapeDrawable* boxDrawable = new osg::ShapeDrawable(box);
                boxGeode->addDrawable(boxDrawable);
                double c = 1-(i%2)*0.4;
                boxDrawable->setColor( osg::Vec4(c,c,c,0) );
                osg::PositionAttitudeTransform * transform = new osg::PositionAttitudeTransform();
                root->addChild(transform);
                transform->addChild(boxGeode);

                // Store transform for later
                transforms.push_back(transform);
            }

            // Enable lighting
            root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);

            // Init viewer
            viewer->setSceneData( root );
            viewer->setCameraManipulator(new osgGA::TrackballManipulator());
            viewer->realize();

        }
    #endif

    // Time loop
    for (int i = 0; i < NT; ++i){

        // Simulation time
        double t = i*dt;

        // Set connector values
        for (int j = 0; j < N; ++j){
            RigidBody * body = bodies[j];
            Connector * conn = connectors[j];
            conn->m_position       .copy(body->m_position);
            conn->m_quaternion     .copy(body->m_quaternion);
            conn->m_velocity       .copy(body->m_velocity);
            conn->m_angularVelocity.copy(body->m_angularVelocity);
        }

        // Must be called whenever connector values are changed
        solver.updateConstraints();

        if( eqs.size() > 0 ) {

            // Get future velocities - without setting constraint forces
            for (int j = 0; j < N; ++j){
                RigidBody * body = bodies[j];
                Connector * conn = connectors[j];

                //printf("t=%f w=(%g %g %g)\n",t,body->m_angularVelocity[0],body->m_angularVelocity[1],body->m_angularVelocity[2]);

                body->saveState();
                body->integrate(dt);
                conn->setFutureVelocity(body->m_velocity,body->m_angularVelocity);
                body->restoreState();

                //printf("t=%f w=(%g %g %g)\n",t,body->m_angularVelocity[0],body->m_angularVelocity[1],body->m_angularVelocity[2]);
            }

            // Get jacobian information
            for (int j = 0; j < eqs.size(); ++j){
                Equation * eq = eqs[j];

                for (Connector *conn : eq->m_connectors) {
                    RigidBody * body = (RigidBody *)conn->m_userData;

                    Vec3 spatSeed = eq->jacobianElementForConnector(conn).getSpatial(),
                         rotSeed  = eq->jacobianElementForConnector(conn).getRotational(),
                         ddSpatial,
                         ddRotational;

                    // Set jacobians
                    body->getDirectionalDerivative(ddSpatial,ddRotational,body->m_position,spatSeed,rotSeed, dt);
                    int I = conn->m_index;
                    int J = eq->m_index;
                    JacobianElement &el = solver.m_mobilities[std::make_pair(I,J)];
                    el.setSpatial(ddSpatial);
                    el.setRotational(ddRotational);
                }
            }

            // Solve system
            solver.solve(true, debug);

            // Add resulting constraint forces to the bodies
            for (int j = 0; j < slaves.size(); ++j){
                //printf("%f\n",slaves[j]->getConnector(0)->m_force[0]);
                bodies[j]->m_force .copy(slaves[j]->getConnector(0)->m_force );
                bodies[j]->m_torque.copy(slaves[j]->getConnector(0)->m_torque);

                //printf("f[%d] = (%g %g %g)\n",j,bodies[j]->m_force[0], bodies[j]->m_force[1], bodies[j]->m_force[2]);
                //printf("t[%d] = (%g %g %g)\n",j,bodies[j]->m_torque[0],bodies[j]->m_torque[1],bodies[j]->m_torque[2]);
            }
        }

        // Integrate bodies
        for (int j = 0; j < N; ++j){
            RigidBody * body = bodies[j];
            //printf("integrating body %d...\n",j);
            body->integrate(dt);
        }

        // Print results
        if(enableCSV) printCSVRow(t,bodies);

        #ifdef SC_USE_OSG
            if(render && !viewer->done()){

                double t = viewer->elapsedTime();

                // Update box transforms
                for(int j=0; j<bodies.size(); j++){
                    Vec3 p = bodies[j]->m_position;
                    Quat q = bodies[j]->m_quaternion;
                    transforms[j]->setAttitude(osg::Quat(q[0],q[1],q[2],q[3]));
                    transforms[j]->setPosition(osg::Vec3(p[0],p[1],p[2]));
                }

                // render
                viewer->frame();
            } else if(render && viewer->done()){
                // Escape
                break;
            }
        #endif
    }

    deleteVectorContent(bodies);
    deleteVectorContent(slaves);
    deleteVectorContent(connectors);
    deleteVectorContent(constraints);
}
