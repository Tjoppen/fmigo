#ifndef SCCONSTRAINT_H
#define SCCONSTRAINT_H

#include <vector>
#include "sc/Equation.h"
#include "sc/Connector.h"

namespace sc {

/**
 * @brief Base class for constraints.
 */
class Constraint {

protected:
    std::vector<Equation*> m_equations;
    void addEquation(Equation * eq);
    Connector * m_connA;
    Connector * m_connB;

public:

    Constraint(Connector*, Connector*);
    virtual ~Constraint();

    /// Arbitrary data from the user
    void * userData;


    /// Get number of equations in this constraint
    int getNumEquations();

    /// Get one of the equations
    Equation * getEquation(int i);

    /// Get connectors
    Connector * getConnA();
    Connector * getConnB();

    // Update the internal stuff
    virtual void update();
};

}

#endif
