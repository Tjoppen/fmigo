#include "sc/Connector.h"
#include "sc/BallJointConstraint.h"
#include "sc/Equation.h"
#include "stdio.h"
#include "stdlib.h"

using namespace sc;

BallJointConstraint::BallJointConstraint(
    Connector* connA,
    Connector* connB,
    const Vec3& localAnchorA,
    const Vec3& localAnchorB
) : Constraint(connA,connB){
    addEquation(&m_x);
    addEquation(&m_y);
    addEquation(&m_z);

    m_localAnchorA.copy(localAnchorA);
    m_localAnchorB.copy(localAnchorB);

    m_x.setConnectors(connA,connB);
    m_y.setConnectors(connA,connB);
    m_z.setConnectors(connA,connB);
    m_x.setDefault();
    m_y.setDefault();
    m_z.setDefault();
}

BallJointConstraint::~BallJointConstraint(){}

void BallJointConstraint::update(){
    // 3 equations

    // Gx = [ -x   -(ri x x)   x   (rj x x)]
    // Gy = [ -y   -(ri x y)   y   (rj x y)]
    // Gz = [ -z   -(ri x z)   z   (rj x z)]

    // or:

    // G = [    -I   -ri*    I   rj*    ]

    Vec3 x(1,0,0);
    Vec3 y(0,1,0);
    Vec3 z(0,0,1);

    // Get world oriented attachment vectors
    Vec3 ri = m_connA->m_quaternion.multiplyVector(m_localAnchorA);
    Vec3 rj = m_connB->m_quaternion.multiplyVector(m_localAnchorB);

    //printf("ri=%g %g %g\n", ri[0], ri[1], ri[2]);
    //printf("rj=%g %g %g\n", rj[0], rj[1], rj[2]);

    // g = ( xj + rj - xi - ri ) . dot ( x )
    // gdot = ( vj + wj.cross(rj) - vi - wi.cross(ri) ) . dot ( x )
    //      = [   -x   -ri.cross(x)   x   rj.cross(x) ] * [ vi wi vj wj ]'
    //      = G * W
    //      ... and same for y and z

    Vec3 gvec = m_connB->m_position + rj - m_connA->m_position - ri;
    m_x.setViolation(gvec.dot(x));
    m_y.setViolation(gvec.dot(y));
    m_z.setViolation(gvec.dot(z));

    //printf("gvec=%g %g %g\n", gvec[0], gvec[1], gvec[2]);

    Vec3 ri_x_x = ri.cross(x);
    Vec3 ri_x_y = ri.cross(y);
    Vec3 ri_x_z = ri.cross(z);

    Vec3 rj_x_x = rj.cross(x);
    Vec3 rj_x_y = rj.cross(y);
    Vec3 rj_x_z = rj.cross(z);

    //printf("ri_x_z=%g %g %g\n", ri_x_z[0], ri_x_z[1], ri_x_z[2]);

    m_x.setG(-1, 0, 0, -ri_x_x.x(), -ri_x_x.y(), -ri_x_x.z(),    1, 0, 0,  rj_x_x.x(), rj_x_x.y(), rj_x_x.z());
    m_y.setG( 0,-1, 0, -ri_x_y.x(), -ri_x_y.y(), -ri_x_y.z(),    0, 1, 0,  rj_x_y.x(), rj_x_y.y(), rj_x_y.z());
    m_z.setG( 0, 0,-1, -ri_x_z.x(), -ri_x_z.y(), -ri_x_z.z(),    0, 0, 1,  rj_x_z.x(), rj_x_z.y(), rj_x_z.z());

}
