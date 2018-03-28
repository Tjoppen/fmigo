#include "sc/Mat3.h"

using namespace sc;

Mat3::Mat3(){
    set(0,0,0,
        0,0,0,
        0,0,0);
}

Mat3::Mat3(const Quat& q){
    setRotationFromQuaternion(q);
}

Mat3::~Mat3(){}

void Mat3::set( double a11, double a12, double a13,
                double a21, double a22, double a23,
                double a31, double a32, double a33){
    m_data[0] = a11;
    m_data[1] = a12;
    m_data[2] = a13;
    m_data[3] = a21;
    m_data[4] = a22;
    m_data[5] = a23;
    m_data[6] = a31;
    m_data[7] = a32;
    m_data[8] = a33;
}

Mat3 Mat3::transpose() const {
    Mat3 result;
    for(int i=0; i<3; i++){
        for(int j=0; j<3; j++){
            result.setElement(i,j, this->getElement(j,i));
        }
    }
    return result;
}

void Mat3::copy(const Mat3& m){
    for(int i=0; i<3; i++)
        for(int j=0; j<3; j++)
            this->setElement(i,j,m.getElement(i,j));
}

void Mat3::setRotationFromQuaternion(const Quat& q){
    double  x = q.x(), y = q.y(), z = q.z(), w = q.w(),
            x2 = x + x, y2 = y + y, z2 = z + z,
            xx = x * x2, xy = x * y2, xz = x * z2,
            yy = y * y2, yz = y * z2, zz = z * z2,
            wx = w * x2, wy = w * y2, wz = w * z2;

    set(1 - ( yy + zz ) ,   xy - wz,            xz + wy,
        xy + wz         ,   1 - ( xx + zz ),    yz - wx,
        xz - wy         ,   yz + wx,            1 - ( xx + yy ) );
}

Vec3 Mat3::multiplyVector(const Vec3& v) const {
    return Vec3(getColumn(0).dot(v), getColumn(1).dot(v), getColumn(2).dot(v));
};

Vec3 Mat3::getColumn(int i) const {
    return Vec3(m_data[3*0 + i],m_data[3*1 + i],m_data[3*2 + i]);
};

Mat3 Mat3::multiplyMatrix(const Mat3& m) const {
    Mat3 result;
    for(int i=0; i<3; i++){
        for(int j=0; j<3; j++){
            double sum = 0.0;
            for(int k=0; k<3; k++){
                sum += m.getElement(k,i) * this->getElement(j,k);
            }
            result.setElement(j,i,sum);
        }
    }
    return result;
};

Mat3 Mat3::scale(const Vec3& v) {
    Mat3 result;
    for(int i=0; i<3; i++){
        result.setElement(i,0, v.x() * this->getElement(i,0));
        result.setElement(i,1, v.y() * this->getElement(i,1));
        result.setElement(i,2, v.z() * this->getElement(i,2));
    }
    return result;
};

double Mat3::getElement(int row, int column) const {
    return m_data[3*row + column];
}

void Mat3::setElement(int row, int column, double value){
    m_data[3*row + column] = value;
}
