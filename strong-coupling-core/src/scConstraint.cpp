#include "scConnector.h"
#include "scConstraint.h"
#include "stdio.h"

scConstraint::scConstraint(scConnector* connA, scConnector* connB){
    m_connA = connA;
    m_connB = connB;
}

scConstraint::~scConstraint(){
}

int scConstraint::getNumEquations(){
    return m_equations.size();
}

scEquation * scConstraint::getEquation(int i){
    return m_equations[i];
}
