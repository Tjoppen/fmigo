/*
 * MultiWayConstraint.h
 *
 *  Created on: Mar 16, 2018
 *      Author: thardin
 */

#ifndef MULTIWAYCONSTRAINT_H_
#define MULTIWAYCONSTRAINT_H_

#include "sc/Constraint.h"

namespace sc {
/**
 * @brief A bit like ShaftConstraint, except between more than two nodes and with associated weights.
 * The purpose of the weights is to be able to make things like a car differential as a constraint.
 * Some examples:
 *
 *  a = b           -> weights = {1, -1} or {-1, 1}     (ShaftConstraint)
 *  a = b + c       -> weights = {-1, 1, 1}             ungeared differential
 *  a = 2*(b+c)     -> weights = {-1, 2, 2}             geared differential
 *  a + b + c = 0   -> weights = {1, 1, 1}              Kirchoff's current law
 */
class MultiWayConstraint : public Constraint {
private:
    Equation m_eq;
    std::vector<double> m_weights;

public:
    MultiWayConstraint(const std::vector<Connector*>& connectors, const std::vector<double>& weights);

    virtual ~MultiWayConstraint();
    void update();
};
}

#endif /* MULTIWAYCONSTRAINT_H_ */
