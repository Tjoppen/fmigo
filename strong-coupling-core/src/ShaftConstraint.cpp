/*
 * ShaftConstraint.cpp
 *
 *  Created on: Aug 22, 2014
 *      Author: thardin
 */

#include "sc/ShaftConstraint.h"

using namespace sc;

ShaftConstraint::ShaftConstraint(Connector* connA, Connector* connB, int axis) : Constraint(connA, connB) {
    addEquation(&m_eq);

    //our model is that both shafts run along either the X, Y or Z axis
    if      (axis == 0) m_eq.setG( 0, 0, 0,-1, 0, 0, 0, 0, 0, 1, 0, 0);
    else if (axis == 1) m_eq.setG( 0, 0, 0, 0,-1, 0, 0, 0, 0, 0, 1, 0);
    else                m_eq.setG( 0, 0, 0, 0, 0,-1, 0, 0, 0, 0, 0, 1);

    m_eq.setConnectors(connA,connB);
    m_eq.setDefault();
}

ShaftConstraint::~ShaftConstraint() {
}

void ShaftConstraint::update() {
    //our violation is simply the difference in angle between the two shafts
    m_eq.setViolation(m_connB->m_shaftAngle - m_connA->m_shaftAngle);
}
