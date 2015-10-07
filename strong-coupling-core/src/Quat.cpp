#include "sc/Quat.h"
#include "sc/Vec3.h"
#include "math.h"
#include "stdio.h"

using namespace sc;

Quat::Quat(){
    m_data[0] = 0; // x
    m_data[1] = 0; // y
    m_data[2] = 0; // z

    m_data[3] = 1; // w
}
Quat::Quat( double x, double y, double z, double w){
    m_data[0] = x;
    m_data[1] = y;
    m_data[2] = z;

    m_data[3] = w;
}
Quat::~Quat(){}

double Quat::x() const {
    return m_data[0];
}
double Quat::y() const {
    return m_data[1];
}
double Quat::z() const {
    return m_data[2];
}
double Quat::w() const {
    return m_data[3];
}

Vec3 Quat::getAxis() const {
    return Vec3(m_data[0],m_data[1],m_data[2]);
};

Quat Quat::multiply(const Quat& q) const {
    Vec3 va(m_data[0],m_data[1],m_data[2]);
    Vec3 vb(q[0],q[1],q[2]);

    double wa = m_data[3],
           wb = q[3];

    Vec3 vaxvb = va.cross(vb);

    /*
    target.x = w * vb.x + q.w*va.x + vaxvb.x;
    target.y = w * vb.y + q.w*va.y + vaxvb.y;
    target.z = w * vb.z + q.w*va.z + vaxvb.z;
    target.w = w*q.w - va.dot(vb);
    */

    return Quat(wa * vb[0] + wb*va[0] + vaxvb[0],
                wa * vb[1] + wb*va[1] + vaxvb[1],
                wa * vb[2] + wb*va[2] + vaxvb[2],
                wa * wb - va.dot(vb));

}

Vec3 Quat::multiplyVector(const Vec3& v) const {
    Vec3 result;

    double  x = v.x(),
            y = v.y(),
            z = v.z();

    double  qx = this->x(),
            qy = this->y(),
            qz = this->z(),
            qw = this->w();

    // q*v
    double  ix =  qw * x + qy * z - qz * y,
            iy =  qw * y + qz * x - qx * z,
            iz =  qw * z + qx * y - qy * x,
            iw = -qx * x - qy * y - qz * z;

    result.set( ix * qw + iw * -qx + iy * -qz - iz * -qy,
                iy * qw + iw * -qy + iz * -qx - ix * -qz,
                iz * qw + iw * -qz + ix * -qy - iy * -qx);

    return result;
}

void Quat::normalize(){
    double l = sqrt(m_data[0]*m_data[0]+m_data[1]*m_data[1]+m_data[2]*m_data[2]+m_data[3]*m_data[3]);
    if ( l == 0 ) {
        m_data[0] = 0;
        m_data[1] = 0;
        m_data[2] = 0;
        m_data[3] = 1;
    } else {
        l = 1 / l;
        m_data[0] *= l;
        m_data[1] *= l;
        m_data[2] *= l;
        m_data[3] *= l;
    }
}

void Quat::set(double x, double y, double z, double w){
    m_data[0] = x;
    m_data[1] = y;
    m_data[2] = z;
    m_data[3] = w;
}

void Quat::copy(const Quat& q){
    m_data[0] = q[0];
    m_data[1] = q[1];
    m_data[2] = q[2];
    m_data[3] = q[3];
}
