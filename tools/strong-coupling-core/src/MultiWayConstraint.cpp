/*
 * MultiWayConstraint.cpp
 *
 *  Created on: Mar 16, 2018
 *      Author: thardin
 */

#include "sc/MultiWayConstraint.h"

using namespace sc;

MultiWayConstraint::MultiWayConstraint(const std::vector<Connector*>& connectors,
                                       const std::vector<double>& weights) :
        Constraint(connectors),
        m_eq(connectors),
        m_weights(weights)
{
    addEquation(&m_eq);

    //our model is that both shafts run along either the X, Y or Z axis
    for (size_t i = 0; i < weights.size(); i++) {
        m_eq.setG(i, 0, 0, 0, weights[i], 0, 0);
    }
    m_eq.m_isSpatial = false; //rotational only
}

MultiWayConstraint::~MultiWayConstraint() {
}

void MultiWayConstraint::update() {
    //our violation is simply the difference in angle between the two shafts
    //TODO: is this holonomic?
    double g = 0;
    for (size_t i = 0; i < m_weights.size(); i++) {
        g += m_connectors[i]->m_shaftAngle * m_weights[i];
    }
    m_eq.setViolation(g);
}
