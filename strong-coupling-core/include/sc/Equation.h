#ifndef EQUATION_H
#define EQUATION_H

#include "sc/Connector.h"
#include "sc/JacobianElement.h"

namespace sc {

/// Base class for equations. Constrains two instances of Connector.
class Equation {

private:
    Connector * m_connA;
    Connector * m_connB;
    JacobianElement m_G_A;
    JacobianElement m_G_B;
    JacobianElement m_invMGt_A;
    JacobianElement m_invMGt_B;
    double m_g;
    double m_relativeVelocity;

public:
    //index in system (row/column in S)
    int m_index;

    Equation();
    Equation(Connector*,Connector*);
    virtual ~Equation();

    Connector * getConnA();
    Connector * getConnB();
    void setDefault();
    void setConnA(Connector *);
    void setConnB(Connector *);
    void setConnectors(Connector *,Connector *);

    JacobianElement getGA();
    JacobianElement getGB();
    JacobianElement getddA();
    JacobianElement getddB();

    /// Spook parameter "a"
    double m_a;

    /// Spook parameter "b"
    double m_b;

    /// Spook parameter "epsilon"
    double m_epsilon;

    /// Time step
    double m_timeStep;

    // for figuring out which jacobians we need
    bool m_isSpatial, m_isRotational;

    /**
     * @brief Sets a, b, epsilon according to SPOOK.
     * @param relaxation
     * @param compliance
     * @param timeStep
     */
    void setSpookParams(double relaxation, double compliance, double timeStep);

    /// Get constraint violation, g
    double getViolation();
    void setViolation(double g);
    void setDefaultViolation();

    /// Get constraint velocity, G*W
    double getVelocity();
    void setRelativeVelocity(double);
    double getFutureVelocity();

    /**
     * @brief Set the spatial components of connector A jacobian.
     * @param x
     * @param y
     * @param z
     */
    void setSpatialJacobianA   (double x, double y, double z);
    void setSpatialJacobianA   (const Vec3& seed);

    /// Set the rotational components of connector A jacobian.
    void setRotationalJacobianA(double x, double y, double z);
    void setRotationalJacobianA(const Vec3& seed);

    /// Set the spatial components of connector B jacobian.
    void setSpatialJacobianB   (double x, double y, double z);
    void setSpatialJacobianB   (const Vec3& seed);

    /// Set the rotational components of connector B jacobian.
    void setRotationalJacobianB(double x, double y, double z);
    void setRotationalJacobianB(const Vec3& seed);

    /// Set all jacobian elements at once.
    void setJacobian(double,double,double,double,double,double,double,double,double,double,double,double);

    /**
     * @brief Get the spatial components of connector A jacobian seed.
     * @param seed
     */
    void getSpatialJacobianSeedA   (Vec3& seed);

    /// Get the rotational components of connector A jacobian.
    void getRotationalJacobianSeedA(Vec3& seed);

    /// Get the spatial components of connector B jacobian.
    void getSpatialJacobianSeedB   (Vec3& seed);

    /// Get the rotational components of connector B jacobian.
    void getRotationalJacobianSeedB(Vec3& seed);

    //these return the seed for some connector associated with this Equation
    Vec3 getSpatialJacobianSeed(Connector *conn);
    Vec3 getRotationalJacobianSeed(Connector *conn);

    void setG(  double,double,double,
                double,double,double,
                double,double,double,
                double,double,double);
    void setG(  const Vec3& spatialA,
                const Vec3& rotationalA,
                const Vec3& spatialB,
                const Vec3& rotationalB);

};

}

#endif
