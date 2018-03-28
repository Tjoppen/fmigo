#include "RigidBody.h"
#include "sc/Quat.h"
#include "sc/Vec3.h"
#include "sc/Mat3.h"
#include "stdio.h"

using namespace sc;

RigidBody::RigidBody(){
    m_invMass = 1;
    m_invInertia.set(1,1,1);
    m_invInertiaWorld.set(  1,0,0,
                            0,1,0,
                            0,0,1 );
    m_quaternion.set(0,0,0,1);
}

RigidBody::~RigidBody(){}

void RigidBody::integrate(double dt){

    if (m_invMass == 0)
        return;

    m_force += m_gravity * (1.0/m_invMass);


    // Integrate linear
    m_velocity += m_force * dt * m_invMass;
    m_position += m_velocity * dt;

    /*
    printf("%g %g %g\n%g %g %g\n%g %g %g\n\n",
        m_invInertiaWorld.getElement(0,0),m_invInertiaWorld.getElement(0,1),m_invInertiaWorld.getElement(0,2),
        m_invInertiaWorld.getElement(1,0),m_invInertiaWorld.getElement(1,1),m_invInertiaWorld.getElement(1,2),
        m_invInertiaWorld.getElement(2,0),m_invInertiaWorld.getElement(2,1),m_invInertiaWorld.getElement(2,2));
    */

    // Integrate angular velocity
    m_angularVelocity += m_invInertiaWorld * m_torque * dt;

    Vec3 added = m_invInertiaWorld * m_torque * dt;

    //printf("added = %g %g %g\n", added[0], added[1], added[2]);
    //printf("w = %g %g %g %g\n", m_angularVelocity[0], m_angularVelocity[1], m_angularVelocity[2]);

    // Integrate orientation
    Quat w(m_angularVelocity[0], m_angularVelocity[1], m_angularVelocity[2], 0.0);
    Quat wq = w.multiply(m_quaternion);

    Vec3 x(1,0,0);

    m_quaternion[0] += 0.5 * dt * wq[0];
    m_quaternion[1] += 0.5 * dt * wq[1];
    m_quaternion[2] += 0.5 * dt * wq[2];
    m_quaternion[3] += 0.5 * dt * wq[3];

    m_quaternion.normalize();

    Vec3 new2 = m_quaternion.multiplyVector(x);

    updateWorldInertia();

    resetForces();
}

void RigidBody::resetForces(){
    m_force.set(0,0,0);
    m_torque.set(0,0,0);
}

void RigidBody::setLocalInertiaAsBox(double invMass, const Vec3& halfExtents){
    m_invMass = invMass;
    m_invInertia.set( invMass / (  1.0 / 12.0 * (   2*halfExtents.y()*2*halfExtents.y() + 2*halfExtents.z()*2*halfExtents.z() ) ),
                      invMass / (  1.0 / 12.0 * (   2*halfExtents.x()*2*halfExtents.x() + 2*halfExtents.z()*2*halfExtents.z() ) ),
                      invMass / (  1.0 / 12.0 * (   2*halfExtents.y()*2*halfExtents.y() + 2*halfExtents.x()*2*halfExtents.x() ) )  );
    updateWorldInertia();
}

void RigidBody::updateWorldInertia(){
    // Update world inertia according to new orientation
    Mat3 R(m_quaternion);
    Mat3 Rt = R.transpose();
    R = R.scale(m_invInertia);
    m_invInertiaWorld = R * Rt;
}

void RigidBody::getDirectionalDerivative(   Vec3& outSpatial,
                                            Vec3& outRotational,
                                            Vec3& position,
                                            const Vec3& spatialDirection,
                                            const Vec3& rotationalDirection,
                                            double timeStep){

    Vec3 velo_noforce,
         velo_withforce,
         avelo_noforce,
         avelo_withforce;

    // Just do spatial on the CM for now
    position.copy(m_position);

    // Step with external force
    saveState();
    m_force.set(0,0,0);
    m_torque.set(0,0,0);
    m_force  += spatialDirection;
    m_torque += rotationalDirection;
    integrate(timeStep);
    velo_withforce.copy(m_velocity);
    avelo_withforce.copy(m_angularVelocity);
    restoreState();

    // Step without added force
    saveState();
    m_force.set(0,0,0);
    m_torque.set(0,0,0);
    integrate(timeStep);
    velo_noforce.copy(m_velocity);
    avelo_noforce.copy(m_angularVelocity);
    restoreState();

    // The derivative is difference in velocity
    outSpatial =    velo_withforce .subtract(velo_noforce) * (1.0/timeStep);
    outRotational = avelo_withforce.subtract(avelo_noforce) * (1.0/timeStep);
}

void RigidBody::saveState(){
    m_position2         .copy(m_position);
    m_velocity2         .copy(m_velocity);
    m_force2            .copy(m_force);
    m_torque2           .copy(m_torque);
    m_angularVelocity2  .copy(m_angularVelocity);
    m_quaternion2       .copy(m_quaternion);
    m_gravity2          .copy(m_gravity);
    m_invInertia2       .copy(m_invInertia);
    m_invInertiaWorld2  .copy(m_invInertiaWorld);
    m_invMass2 = m_invMass;
}

void RigidBody::restoreState(){
    m_position       .copy(m_position2);
    m_velocity       .copy(m_velocity2);
    m_force          .copy(m_force2);
    m_torque         .copy(m_torque2);
    m_angularVelocity.copy(m_angularVelocity2);
    m_quaternion     .copy(m_quaternion2);
    m_gravity        .copy(m_gravity2);
    m_invInertia     .copy(m_invInertia2);
    m_invInertiaWorld.copy(m_invInertiaWorld2);
    m_invMass =      m_invMass2;
}

