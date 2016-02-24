/*
 * ShaftConstraint.h
 *
 *  Created on: Aug 22, 2014
 *      Author: thardin
 */

#ifndef SHAFTCONSTRAINT_H_
#define SHAFTCONSTRAINT_H_

#include "sc/Constraint.h"

namespace sc {
/**
 * @brief Locks the accumulated angle of two shafts. Ignores orientation
 */
class ShaftConstraint : public Constraint {
private:
    Equation m_eq;

public:
    ShaftConstraint(Connector* connA, Connector* connB);

    virtual ~ShaftConstraint();
    void update();
};
}

#endif /* SHAFTCONSTRAINT_H_ */
