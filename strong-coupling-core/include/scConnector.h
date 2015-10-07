#ifndef SCCONNECTOR_H
#define SCCONNECTOR_H

/**
 * The connector is a handle in each slave, which can be constrained. The connector
 * could be the slave center of mass (in the rigid body case) or something else.
 */
class scConnector {

private:

public:
    scConnector();
    ~scConnector();

    /// Index of the connector in the system matrix. Will be set by the scSolver it is added to.
    int m_index;

    /// Pointer to arbitrary user data. Use it as you want.
    void * userData;

    /// Physical position of the connector.
    double m_position[3];

    /// Physical (linear) velocity of the connector.
    double m_velocity[3];

    /// Quaternion orientation of this connector. Needed?
    double m_quaternion[4];

    /// Angular velocity of the connector.
    double m_angularVelocity[3];

    /// Resulting contraint force. Will be set by the scSolver in scSolver::solve().
    double m_force[3];

    /// Resulting constraint torque.
    double m_torque[3];

    /// Set the position of this connector
    void setPosition(double x, double y, double z);

    /// Set velocity of the connector
    void setVelocity(double vx, double vy, double vz);

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

#endif
