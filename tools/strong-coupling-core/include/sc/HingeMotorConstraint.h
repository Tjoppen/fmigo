#ifndef SCHINGEMOTORCONSTRAINT_H
#define SCHINGEMOTORCONSTRAINT_H

#include "sc/Constraint.h"

namespace sc {

/**
 * @brief Locks all 6 degrees of freedom between two connectors.
 */
class HingeMotorConstraint : public Constraint {

private:
    Equation m_equation;
    Vec3 m_localAxisA;
    Vec3 m_localAxisB;
public:
    HingeMotorConstraint(Connector* connA,
                         Connector* connB,
                         const Vec3& localAxisA,
                         const Vec3& localAxisB,
                         double relativeVelocity);
    virtual ~HingeMotorConstraint();
    void update();
};

}

#endif
