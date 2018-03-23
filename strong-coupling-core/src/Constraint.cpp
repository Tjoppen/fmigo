#include "sc/Connector.h"
#include "sc/Constraint.h"
#include "stdio.h"

using namespace sc;

Constraint::Constraint(Connector* connA, Connector* connB){
    m_connectors.push_back(connA);
    m_connectors.push_back(connB);
}

Constraint::Constraint(const std::vector<Connector*>& connectors) :
    m_connectors(connectors)
{
}

Constraint::~Constraint(){}

int Constraint::getNumEquations(){
    return m_equations.size();
}

void Constraint::addEquation(Equation * eq){
    //add equation to its own connectors' lists of equations
    for (Connector *c : m_connectors) {
        c->m_equations.push_back(eq);
    }
    m_equations.push_back(eq);
}

Equation * Constraint::getEquation(int i){
    return m_equations[i];
}

void Constraint::update(){}
