#include "sc/Equation.h"

using namespace sc;
using namespace std;

Equation::Equation(const vector<Connector*>& connectors) : m_connectors(connectors) {
    setDefault();
}

Equation::Equation(Connector * connA, Connector * connB){
    m_connectors.push_back(connA);
    m_connectors.push_back(connB);

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

void Equation::setDefaultViolation(){
    Vec3 zero;
    m_g = m_G_A.multiply(m_connectors[0]->m_position, zero) +
          m_G_B.multiply(m_connectors[1]->m_position, zero);
}

void Equation::setViolation(double g){
    m_g = g;
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
    for (Connector *conn1 : m_connectors) {
        for (Connector *conn2 : other->m_connectors) {
            if (conn1->m_slave == conn2->m_slave) {
                return true;
            }
        }
    }
    return false;
}
