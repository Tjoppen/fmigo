#include "master/StrongConnection.h"
#include "master/Master.h"
#include <sc/LockConstraint.h>

using namespace fmitcp_master;

StrongConnection::StrongConnection( StrongConnector* connA, StrongConnector* connB, StrongConnectionType type ) {

    m_constraint = NULL;

    // Check so that all needed connector values are set
    switch(type){

    case CONNECTION_LOCK:
        if(!(
            connA->hasPosition() &&
            connB->hasPosition() &&
            connA->hasQuaternion() &&
            connB->hasQuaternion() &&
            connA->hasVelocity() &&
            connB->hasVelocity() &&
            connA->hasAngularVelocity() &&
            connB->hasAngularVelocity() &&
            connA->hasForce() &&
            connB->hasForce() &&
            connA->hasTorque() &&
            connB->hasTorque()
            )){
            fprintf(stderr, "Lock connections need connectors with positions, velocities and forces.\n");
            exit(1);
        }

        sc::Vec3 localAnchorA(0,0,0);
        sc::Vec3 localAnchorB(0,0,0);
        sc::Quat localOrientationA(0,0,0,1);
        sc::Quat localOrientationB(0,0,0,1);
        m_constraint = new sc::LockConstraint(  connA->getConnector(),connB->getConnector(),
                                                localAnchorA,
                                                localAnchorB,
                                                localOrientationA,
                                                localOrientationB);

        break;

    }
}
StrongConnection::~StrongConnection(){
    if(m_constraint != NULL)
        delete m_constraint;
}


sc::Constraint * StrongConnection::getConstraint(){
    return m_constraint;
}
