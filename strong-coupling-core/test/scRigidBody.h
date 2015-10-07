#ifndef SCRIGIDBODY_H
#define SCRIGIDBODY_H

class scRigidBody {

public:

    scRigidBody();
    ~scRigidBody();

    double m_position[3];
    double m_velocity[3];
    double m_angularVelocity[3];
    double m_force[3];
    double m_torque[3];

    double m_invMass;
    double m_invInertia;

    double m_quaternion[4];
    double m_tmpQuat1[4];
    double m_tmpQuat2[4];

    /// Move forward in time
    void integrate(double dt);
};

#endif
