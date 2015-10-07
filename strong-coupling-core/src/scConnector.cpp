#include "scConnector.h"

scConnector::scConnector(){
    m_index = 0;
    // Set all members to zero
    for (int i = 0; i < 4; ++i){
        m_quaternion[i] = 0;
        if(i<3){
            m_position[i] = 0;
            m_velocity[i] = 0;
            m_angularVelocity[i] = 0;
            m_force[i] = 0;
            m_torque[i] = 0;
        }
    }
}

scConnector::~scConnector(){

}

void scConnector::setPosition(double x, double y, double z){
    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;
}

void scConnector::setVelocity(double vx, double vy, double vz){
    m_velocity[0] = vx;
    m_velocity[1] = vy;
    m_velocity[2] = vz;
}

void scConnector::setOrientation(double x, double y, double z, double w){
    m_quaternion[0] = x;
    m_quaternion[1] = y;
    m_quaternion[2] = z;
    m_quaternion[3] = w;
}

void scConnector::setAngularVelocity(double wx, double wy, double wz){
    m_angularVelocity[0] = wx;
    m_angularVelocity[1] = wy;
    m_angularVelocity[2] = wz;
}

double scConnector::getConstraintForce(int element){
    return 0;
}

double scConnector::getConstraintTorque(int element){
    return 0;
}
