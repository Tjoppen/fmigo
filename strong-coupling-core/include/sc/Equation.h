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
    double m_g;
    double m_relativeVelocity;

public:
    //index in system (row/column in S)
    int m_index;

    Equation();
    Equation(Connector*,Connector*);
    virtual ~Equation();

    std::vector<Connector*> getConnectors() const;
    void setDefault();
    void setConnectors(Connector *,Connector *);

    // for figuring out which jacobians we need
    bool m_isSpatial, m_isRotational;

    /// Get constraint violation, g
    double getViolation();
    void setViolation(double g);
    void setDefaultViolation();

    /// Get constraint velocity, G*W
    double getVelocity();
    void setRelativeVelocity(double);
    double getFutureVelocity();

    void setG(  double,double,double,
                double,double,double,
                double,double,double,
                double,double,double);
    void setG(  const Vec3& spatialA,
                const Vec3& rotationalA,
                const Vec3& spatialB,
                const Vec3& rotationalB);

    bool haveOverlappingFMUs(Equation *other) const;
    JacobianElement& jacobianElementForConnector(Connector *conn);
};

}

#endif
