#include "sc/Connector.h"
#include "sc/Constraint.h"
#include "stdio.h"

using namespace sc;

Constraint::Constraint(Connector* connA, Connector* connB){
    m_connA = connA;
    m_connB = connB;
}

Constraint::~Constraint(){}

int Constraint::getNumEquations(){
    return m_equations.size();
}

void Constraint::addEquation(Equation * eq){
    m_equations.push_back(eq);
}

Equation * Constraint::getEquation(int i){
    return m_equations[i];
}

Connector * Constraint::getConnA(){
    return m_connA;
}

Connector * Constraint::getConnB(){
    return m_connB;
}

void Constraint::update(){}
