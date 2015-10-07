#ifndef SCLOCKCONSTRAINT_H
#define SCLOCKCONSTRAINT_H

#include "scConstraint.h"

/**
 * Locks all degrees of freedom between two connectors.
 */
class scLockConstraint : public scConstraint {

private:

public:
    scLockConstraint(scConnector* connA, scConnector* connB);
    virtual ~scLockConstraint();
};

#endif
