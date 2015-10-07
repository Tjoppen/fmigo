#include "scConnector.h"
#include "scLockConstraint.h"
#include "scEquation.h"
#include "stdio.h"
#include "stdlib.h"

scLockConstraint::scLockConstraint(scConnector* connA, scConnector* connB) : scConstraint(connA,connB){

    // Create 6 equations, one for each DOF
    for (int i = 0; i < 6; ++i){
        scEquation * eq = new scEquation(connA,connB);
        eq->m_G[i] = 1;
        eq->m_G[i + 6] = -1;
        m_equations.push_back(eq);
    }
}

scLockConstraint::~scLockConstraint() {
    // Deallocate all equations
    while(m_equations.size() > 0){
        delete m_equations.back();
        m_equations.pop_back();
    }
}
