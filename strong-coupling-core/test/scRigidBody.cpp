#include "scRigidBody.h"
#include "scQuat.h"
#include "scVec3.h"
#include "stdio.h"

scRigidBody::scRigidBody(){
    m_invMass = 1;
    m_invInertia = 1;

    scQuat::set(m_quaternion,0,0,0,1);
    scQuat::set(m_tmpQuat1,0,0,0,1);
    scQuat::set(m_tmpQuat2,0,0,0,1);

    scVec3::set(m_position,0,0,0);
    scVec3::set(m_velocity,0,0,0);
    scVec3::set(m_angularVelocity,0,0,0);
    scVec3::set(m_force,0,0,0);
    scVec3::set(m_torque,0,0,0);
}

scRigidBody::~scRigidBody(){}

void scRigidBody::integrate(double dt){
    for (int i = 0; i < 3; ++i){

        // Linear
        m_velocity[i] += m_invMass * m_force[i] * dt;
        m_position[i] += dt * m_velocity[i];

        // Angular
        m_angularVelocity[i] += m_invInertia * m_torque[i] * dt;
        m_position[i] += dt * m_angularVelocity[i];
    }

    scQuat::set(m_tmpQuat1, m_angularVelocity[0], m_angularVelocity[1], m_angularVelocity[2], 0.0);

    scQuat::multiply(m_tmpQuat1, m_quaternion, m_tmpQuat2);

    m_quaternion[0] += 0.5 * dt * m_tmpQuat2[0];
    m_quaternion[1] += 0.5 * dt * m_tmpQuat2[1];
    m_quaternion[2] += 0.5 * dt * m_tmpQuat2[2];
    m_quaternion[3] += 0.5 * dt * m_tmpQuat2[3];

    scQuat::normalize(m_quaternion,m_quaternion);
}
