#ifndef SCHINGECONSTRAINT_H
#define SCHINGECONSTRAINT_H

#include "sc/BallJointConstraint.h"

namespace sc {

/**
 * @brief Locks all 6 degrees of freedom between two connectors.
 */
class HingeConstraint : public BallJointConstraint {

private:
    Equation m_equationA;
    Equation m_equationB;
    Vec3 m_localPivotA;
    Vec3 m_localPivotB;
public:
    HingeConstraint( Connector* connA,
                     Connector* connB,
                     const Vec3& localAnchorA,
                     const Vec3& localAnchorB,
                     const Vec3& localPivotA,
                     const Vec3& localPivotB);
    virtual ~HingeConstraint();
    void update();
};

}

#endif
