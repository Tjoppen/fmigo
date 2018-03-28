#ifndef SCLOCKCONSTRAINT_H
#define SCLOCKCONSTRAINT_H

#include "sc/BallJointConstraint.h"

namespace sc {

/**
 * @brief Locks all 6 degrees of freedom between two connectors.
 */
class LockConstraint : public BallJointConstraint {

private:
    Equation m_xr;
    Equation m_yr;
    Equation m_zr;

public:
    LockConstraint(Connector* connA, Connector* connB);
    LockConstraint( Connector* connA,
                    Connector* connB,
                    const Vec3& localAnchorA,
                    const Vec3& localAnchorB,
                    const Quat& localOrientationA,
                    const Quat& localOrientationB );

    virtual ~LockConstraint();
    void update();
    void init(  Connector* connA,
                Connector* connB,
                const Vec3& localAnchorA,
                const Vec3& localAnchorB,
                const Quat& localOrientationA,
                const Quat& localOrientationB );
};

}

#endif
