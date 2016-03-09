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

Connector * Equation::getConnA(){ return m_connA; }
Connector * Equation::getConnB(){ return m_connB; }

vector<Connector*> Equation::getConnectors() {
    vector<Connector*> ret;
    ret.push_back(m_connA);
    ret.push_back(m_connB);
    return ret;
}

void Equation::setConnA(Connector* c){ m_connA = c; }
void Equation::setConnB(Connector* c){ m_connB = c; }
void Equation::setConnectors(Connector* cA, Connector* cB){ setConnA(cA);setConnB(cB); }
JacobianElement Equation::getGA(){ return m_G_A; }
JacobianElement Equation::getGB(){ return m_G_B; }
JacobianElement Equation::getddA(){ return m_invMGt_A; }
JacobianElement Equation::getddB(){ return m_invMGt_B; }

void Equation::setSpookParams(double relaxation, double compliance, double timeStep){
    m_a = 4/(1+4*relaxation)/timeStep;
    m_b = 1/(1+4*relaxation);
    m_epsilon = 4 * compliance / (timeStep*timeStep * (1 + 4*relaxation));
    m_timeStep = timeStep;
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

void Equation::setJacobian( double G1,
                            double G2,
                            double G3,
                            double G4,
                            double G5,
                            double G6,
                            double G7,
                            double G8,
                            double G9,
                            double G10,
                            double G11,
                            double G12 ){
    setSpatialJacobianA(G1,G2,G3);
    setRotationalJacobianA(G4,G5,G6);
    setSpatialJacobianB(G7,G8,G9);
    setRotationalJacobianB(G10,G11,G12);
}

void Equation::setSpatialJacobianA(double x, double y, double z){
    m_invMGt_A.setSpatial(x,y,z);
}

void Equation::setSpatialJacobianA(const Vec3& seed){
    setSpatialJacobianA(seed.x(),seed.y(),seed.z());
}

void Equation::setRotationalJacobianA(double x, double y, double z){
    m_invMGt_A.setRotational(x,y,z);
}

void Equation::setRotationalJacobianA(const Vec3& seed){
    setRotationalJacobianA(seed.x(),seed.y(),seed.z());
}

void Equation::setSpatialJacobianB(double x, double y, double z){
    m_invMGt_B.setSpatial(x,y,z);
}

void Equation::setSpatialJacobianB(const Vec3& seed){
    setSpatialJacobianB(seed.x(),seed.y(),seed.z());
}

void Equation::setRotationalJacobianB(double x, double y, double z){
    m_invMGt_B.setRotational(x,y,z);
}

void Equation::setRotationalJacobianB(const Vec3& seed){
    setRotationalJacobianB( seed.x(),
                            seed.y(),
                            seed.z());
}

void Equation::getSpatialJacobianSeedA(Vec3& seed){
    seed.copy(m_G_A.getSpatial());
}

void Equation::getRotationalJacobianSeedA(Vec3& seed){
    seed.copy(m_G_A.getRotational());
}

void Equation::getSpatialJacobianSeedB(Vec3& seed){
    seed.copy(m_G_B.getSpatial());
}

void Equation::getRotationalJacobianSeedB(Vec3& seed){
    seed.copy(m_G_B.getRotational());
}

Vec3 Equation::getSpatialJacobianSeed(Connector *conn) {
    //here is where we'd generalize this thing if we wanted more than two connectors per equation
    if (conn == m_connA) {
        return m_G_A.getSpatial();
    } else if (conn == m_connB) {
        return m_G_B.getSpatial();
    } else {
        fprintf(stderr, "Attempted to getSpatialJacobianSeed() with connector which is not part of the Equation\n");
        exit(1);
    }
}

Vec3 Equation::getRotationalJacobianSeed(Connector *conn) {
    //fprintf(stderr, "getRotationalJacobianSeed: %p vs %p %p\n", conn, m_connA, m_connB);
    if (conn == m_connA) {
        return m_G_A.getRotational();
    } else if (conn == m_connB) {
        return m_G_B.getRotational();
    } else {
        fprintf(stderr, "Attempted to getRotationalJacobianSeed() with connector which is not part of the Equation\n");
        exit(1);
    }
}
