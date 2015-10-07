#ifndef SCEQUATION_H
#define SCEQUATION_H

#include "scConnector.h"

/**
 * Base class for equations. Constrains two instances of scConnector.
 */
class scEquation {

private:
    scConnector * m_connA;
    scConnector * m_connB;

public:

    scEquation(scConnector*,scConnector*);
    ~scEquation();

    scConnector * getConnA();
    scConnector * getConnB();

    double m_G[12];         // TODO: Should be sparse!
    double m_invMGt[12];    // TODO: Should be sparse!

    /// Spook parameter "a"
    double m_a;

    /// Spook parameter "b"
    double m_b;

    /// Spook parameter "epsilon"
    double m_epsilon;

    /// Time step
    double m_timeStep;

    /**
     * @brief Sets a, b, epsilon according to SPOOK.
     * @param relaxation
     * @param compliance
     * @param timeStep
     */
    void setSpookParams(double relaxation, double compliance, double timeStep);

    /// Get constraint violation, g
    double getViolation();

    /// Get constraint velocity, G*W
    double getVelocity();

    /// Multiply the 12 equation elements (G) with another 12-element vector
    double Gmult(double x1[], double v1[], double x2[], double v2[]);

    /// Multiply the 12 equation elements (G) with another 12-element vector
    double GmultG(double G[]);

    /**
     * @brief Set the spatial components of connector A jacobian.
     * @param x
     * @param y
     * @param z
     */
    void setSpatialJacobianA   (double x, double y, double z);
    void setSpatialJacobianA   (double * seed);

    /// Set the rotational components of connector A jacobian.
    void setRotationalJacobianA(double x, double y, double z);
    void setRotationalJacobianA(double * seed);

    /// Set the spatial components of connector B jacobian.
    void setSpatialJacobianB   (double x, double y, double z);
    void setSpatialJacobianB   (double * seed);

    /// Set the rotational components of connector B jacobian.
    void setRotationalJacobianB(double x, double y, double z);
    void setRotationalJacobianB(double * seed);

    /// Set all jacobian elements at once.
    void setJacobian(double,double,double,double,double,double,double,double,double,double,double,double);

    /**
     * @brief Get the spatial components of connector A jacobian seed.
     * @param seed
     */
    void getSpatialJacobianSeedA   (double * seed);

    /// Get the rotational components of connector A jacobian.
    void getRotationalJacobianSeedA(double * seed);

    /// Get the spatial components of connector B jacobian.
    void getSpatialJacobianSeedB   (double * seed);

    /// Get the rotational components of connector B jacobian.
    void getRotationalJacobianSeedB(double * seed);

};


#endif
