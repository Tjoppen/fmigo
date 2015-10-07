#include "sc/Connector.h"
#include "sc/HingeMotorConstraint.h"
#include "sc/Equation.h"
#include "stdio.h"
#include "stdlib.h"

using namespace sc;

HingeMotorConstraint::~HingeMotorConstraint(){}

HingeMotorConstraint::HingeMotorConstraint(
    Connector* connA,
    Connector* connB,
    const Vec3& localAxisA,
    const Vec3& localAxisB,
    double relativeVelocity
) : Constraint(connA,connB) {
    addEquation(&m_equation);
    m_equation.setRelativeVelocity(relativeVelocity);
    m_equation.setConnectors(connA,connB);
    m_equation.setDefault();
    m_localAxisA = localAxisA;
    m_localAxisB = localAxisB;
}

void HingeMotorConstraint::update(){
    // Rotational violation:
    //      gdot = ni.dot( wi ) - nj.dot( wj ) - relSpeed
    //           = [ 0  ni  0  -nj ] * [ vi wi vj wj ]'
    //           = G * W

    // We don't have positional violations, just velocity
    m_equation.setViolation(0);

    // Compute world oriented axes
    Vec3 ni = m_connA->m_quaternion . multiplyVector( m_localAxisA );
    Vec3 nj = m_connB->m_quaternion . multiplyVector( m_localAxisB );

    Vec3 zero(0,0,0);
    m_equation.setG(zero, ni, zero, nj*(-1));
}
