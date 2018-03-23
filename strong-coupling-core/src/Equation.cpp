#include "sc/Equation.h"

using namespace sc;
using namespace std;

Equation::Equation(const vector<Connector*>& connectors) :
        m_Gs(connectors.size(), JacobianElement()),
        m_connectors(connectors) {
    if (connectors.size() < 2) {
        fprintf(stderr, "Too few connectors in Equation: %zu\n", connectors.size());
        exit(1);
    }
    setDefault();
}

Equation::Equation(Connector * connA, Connector * connB) :
        m_Gs(2, JacobianElement()) {
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

void Equation::setViolation(double g){
    m_g = g;
}

void Equation::setG(double sxA, double syA, double szA,
                    double rxA, double ryA, double rzA,
                    double sxB, double syB, double szB,
                    double rxB, double ryB, double rzB){
    m_Gs[0].setSpatial    (sxA, syA, szA);
    m_Gs[0].setRotational (rxA, ryA, rzA);
    m_Gs[1].setSpatial    (sxB, syB, szB);
    m_Gs[1].setRotational (rxB, ryB, rzB);
}

void Equation::setG(const Vec3& spatialA,
                    const Vec3& rotationalA,
                    const Vec3& spatialB,
                    const Vec3& rotationalB){
    m_Gs[0].setSpatial(spatialA);
    m_Gs[0].setRotational(rotationalA);
    m_Gs[1].setSpatial(spatialB);
    m_Gs[1].setRotational(rotationalB);
}

void Equation::setG(size_t i,
                    double sxA, double syA, double szA,
                    double rxA, double ryA, double rzA) {
    if (i < 0 || i >= m_Gs.size()) {
        fprintf(stderr, "Equation::setG(): i out of range\n");
        exit(1);
    }
    m_Gs[i].setSpatial    (sxA, syA, szA);
    m_Gs[i].setRotational (rxA, ryA, rzA);
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
