#include "sc/Equation.h"
#include "stdio.h"

using namespace sc;
using namespace std;

Equation::Equation(){
    setDefault();
}

Equation::Equation(Connector * connA, Connector * connB){
    m_connA = connA;
    m_connB = connB;

    // Set default solver params
    setDefault();
}

Equation::~Equation(){}

void Equation::setDefault(){
    m_relativeVelocity = 0;
    m_isSpatial = true;
    m_isRotational = true;
}

void Equation::setRelativeVelocity(double v){
    m_relativeVelocity = v;
}

vector<Connector*> Equation::getConnectors() const {
    vector<Connector*> ret;
    ret.push_back(m_connA);
    ret.push_back(m_connB);
    return ret;
}

void Equation::setConnectors(Connector* cA, Connector* cB) {
    m_connA = cA;
    m_connB = cB;
}

void Equation::setDefaultViolation(){
    Vec3 zero;
    m_g = m_G_A.multiply(m_connA->m_position, zero) +
          m_G_B.multiply(m_connB->m_position, zero);
}

double Equation::getViolation(){
    return m_g;
}

void Equation::setViolation(double g){
    m_g = g;
}

double Equation::getVelocity(){
    return  m_G_A.multiply(m_connA->m_velocity, m_connA->m_angularVelocity) +
            m_G_B.multiply(m_connB->m_velocity, m_connB->m_angularVelocity) + m_relativeVelocity;
}

double Equation::getFutureVelocity(){
    return  m_G_A.multiply(m_connA->m_futureVelocity, m_connA->m_futureAngularVelocity) +
            m_G_B.multiply(m_connB->m_futureVelocity, m_connB->m_futureAngularVelocity);
}

void Equation::setG(double sxA, double syA, double szA,
                    double rxA, double ryA, double rzA,
                    double sxB, double syB, double szB,
                    double rxB, double ryB, double rzB){
    m_G_A.setSpatial    (sxA, syA, szA);
    m_G_A.setRotational (rxA, ryA, rzA);
    m_G_B.setSpatial    (sxB, syB, szB);
    m_G_B.setRotational (rxB, ryB, rzB);
}

void Equation::setG(const Vec3& spatialA,
                    const Vec3& rotationalA,
                    const Vec3& spatialB,
                    const Vec3& rotationalB){
    m_G_A.setSpatial(spatialA);
    m_G_A.setRotational(rotationalA);
    m_G_B.setSpatial(spatialB);
    m_G_B.setRotational(rotationalB);
}

bool Equation::haveOverlappingFMUs(Equation *other) const {
    //quadratic complexity, but only used once in Solver
    for (Connector *conn1 : getConnectors()) {
        for (Connector *conn2 : other->getConnectors()) {
            if (conn1->m_slave == conn2->m_slave) {
                return true;
            }
        }
    }
    return false;
}

JacobianElement& Equation::jacobianElementForConnector(Connector *conn) {
    if (conn == m_connA) {
        return m_G_A;
    } else if (conn == m_connB) {
        return m_G_B;
    } else {
        fprintf(stderr, "Attempted to jacobianElementForConnector() with connector which is not part of the Equation\n");
        exit(1);
    }
}
