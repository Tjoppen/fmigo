#ifndef SCCONNECTOR_H
#define SCCONNECTOR_H

#include "sc/Vec3.h"
#include "sc/Quat.h"

namespace sc {

/**
 * @brief The connector is a handle in each slave, which can be constrained.
 * The connector could be the slave center of mass (in the rigid body case) or something else. Depends on your subsystem.
 */
class Connector {

private:

public:
    Connector();
    virtual ~Connector();

    /// Index of the connector in the system matrix. Will be set by the scSolver it is added to.
    int m_index;

    /// Pointer to arbitrary user data. Use it as you want.
    void * m_userData;

    /// Pointer to the owner slave
    void * m_slave; // problems with #include'ing scSlave in here, how to fix?

    /// Physical position of the connector.
    Vec3 m_position;

    /// Physical (linear) velocity of the connector.
    Vec3 m_velocity;
    Vec3 m_futureVelocity;
    Vec3 m_futureAngularVelocity;

    /// Quaternion orientation of this connector. Needed?
    Quat m_quaternion;

    /// For connecting shafts rotating at high speed where quaternions aren't enough to "keep up"
    double m_shaftAngle;

    /// Angular velocity of the connector.
    Vec3 m_angularVelocity;

    /// Resulting contraint force. Will be set by the scSolver in scSolver::solve().
    Vec3 m_force;

    /// Resulting constraint torque.
    Vec3 m_torque;

    /// Set the position of this connector
    void setPosition(double x, double y, double z);

    /// Set velocity of the connector
    void setVelocity(double vx, double vy, double vz);

    void setFutureVelocity(const Vec3& velocity, const Vec3& angularVelocity);

    /// Set orientation of the connector
    void setOrientation(double x, double y, double z, double w);

    /// Set angular velocity of the connector
    void setAngularVelocity(double wx, double wy, double wz);

    /**
     * @brief Get one of the elements of the resulting constraint force.
     * @param  element 0, 1, or 2: the element in the force array to get.
     */
    double getConstraintForce(int element);

    /**
     * @brief Get one of the elements of the resulting constraint torque.
     * @param  element 0, 1, or 2, the element in the torque array to get.
     */
    double getConstraintTorque(int element);
};

}

#endif // SCCONNECTOR_H
