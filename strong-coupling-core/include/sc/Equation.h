#ifndef EQUATION_H
#define EQUATION_H

#include "sc/Connector.h"
#include "sc/JacobianElement.h"
#include "stdio.h"

namespace sc {

/// Base class for equations. Constrains two instances of Connector.
class Equation {

private:
    std::vector<JacobianElement> m_Gs;
    double m_g;
    double m_relativeVelocity;
    void setDefault();

public:
    //index in system (row/column in S)
    int m_index;
    std::vector<Connector*> m_connectors;

    Equation(const std::vector<Connector*>&);
    Equation(Connector*,Connector*);
    virtual ~Equation();

    // for figuring out which jacobians we need
    bool m_isSpatial, m_isRotational;

    /// Get constraint violation, g
    double getViolation() const {
        return m_g;
    }
    void setViolation(double g);

    /// Get constraint velocity, G*W
    void setRelativeVelocity(double);

    double getVelocity() const {
        double ret = m_relativeVelocity;
        for (size_t i = 0; i < m_Gs.size(); i++) {
            ret += m_Gs[i].multiply(m_connectors[i]->m_velocity, m_connectors[i]->m_angularVelocity);
        }
        return ret;
    }

    double getFutureVelocity() const {
        double ret = 0;
        for (size_t i = 0; i < m_Gs.size(); i++) {
            ret += m_Gs[i].multiply(m_connectors[i]->m_futureVelocity, m_connectors[i]->m_futureAngularVelocity);
        }
        return ret;
    }

    //set specific Jacobian element
    void setG(  size_t i,
                double,double,double,
                double,double,double);

    //legacy API
    void setG(  double,double,double,
                double,double,double,
                double,double,double,
                double,double,double);
    //legacy API
    void setG(  const Vec3& spatialA,
                const Vec3& rotationalA,
                const Vec3& spatialB,
                const Vec3& rotationalB);

    bool haveOverlappingFMUs(Equation *other) const;
    JacobianElement& jacobianElementForConnector(Connector *conn) {
        for (size_t i = 0; i < m_Gs.size(); i++) {
            if (conn == m_connectors[i]) {
                return m_Gs[i];
            }
        }
        fprintf(stderr, "Attempted to jacobianElementForConnector() with connector which is not part of the Equation\n");
        exit(1);
    }
};

}

#endif
