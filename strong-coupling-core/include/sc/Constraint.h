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
    std::vector<Connector*> m_connectors;

public:

    Constraint(Connector*, Connector*);
    Constraint(const std::vector<Connector*>& connectors);
    virtual ~Constraint();

    /// Arbitrary data from the user
    void * userData;


    /// Get number of equations in this constraint
    int getNumEquations();

    /// Get one of the equations
    Equation * getEquation(int i);

    // Update the internal stuff
    virtual void update();
};

}

#endif
