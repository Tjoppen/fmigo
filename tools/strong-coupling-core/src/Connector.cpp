#include "sc/Connector.h"

using namespace sc;

Connector::Connector(){
    m_index = 0;
}

Connector::~Connector(){}

void Connector::setPosition(double x, double y, double z){
    m_position.set(x,y,z);
}

void Connector::setVelocity(double vx, double vy, double vz){
    m_velocity.set(vx,vy,vz);
}

void Connector::setFutureVelocity(const Vec3& velocity, const Vec3& angularVelocity){
    m_futureVelocity.copy(velocity);
    m_futureAngularVelocity.copy(angularVelocity);
}

void Connector::setOrientation(double x, double y, double z, double w){
    m_quaternion.set(w,x,y,z);
}

void Connector::setAngularVelocity(double wx, double wy, double wz){
    m_angularVelocity.set(wx,wy,wz);
}

double Connector::getConstraintForce(int element){
    return 0;
}

double Connector::getConstraintTorque(int element){
    return 0;
}
