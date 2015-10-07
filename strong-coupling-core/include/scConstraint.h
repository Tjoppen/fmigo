#ifndef SCCONSTRAINT_H
#define SCCONSTRAINT_H

#include <vector>
#include "scEquation.h"
#include "scConnector.h"

/**
 * Base class for constraints.
 */
class scConstraint {

protected:
    std::vector<scEquation*> m_equations;

public:

    scConstraint(scConnector*, scConnector*);
    virtual ~scConstraint();

    /// Arbitrary data from the user
    void * userData;

    scConnector * m_connA;
    scConnector * m_connB;

    /// Get number of equations in this constraint
    int getNumEquations();

    /// Get one of the equations
    scEquation * getEquation(int i);
};


#endif
