#ifndef SCVEC3_H
#define SCVEC3_H

#include "stdlib.h"

namespace sc {

/// 3D vector class
class Vec3 {
private:
    double m_data[3];

public:
    Vec3();
    Vec3(double x, double y, double z);
    virtual ~Vec3();

    /// Cross product
    Vec3 cross(const Vec3& u) const;

    /// Addition
    Vec3 add(const Vec3& v) const;
    Vec3 subtract(const Vec3& u) const;

    /// Dot product
    double dot(const Vec3& u) const;
    double x() const;
    double y() const;
    double z() const;

    /// Set the elements
    void set(double x, double y, double z);

    /// Copy elements from some other vector
    void copy(const Vec3&);

    void normalize();

    /// Element access
    double& operator[] (const int i) {
        return m_data[i];
    };

    /// u += v
    void operator += (const Vec3& v) {
        this->m_data[0] += v.x();
        this->m_data[1] += v.y();
        this->m_data[2] += v.z();
    };

    /// v * scalar
    Vec3 operator * (double s) const {
        return Vec3(this->m_data[0]*s,
                    this->m_data[1]*s,
                    this->m_data[2]*s);
    };

    /// v + u
    Vec3 operator + (const Vec3 v) const {
        return Vec3(this->m_data[0] + v.x(),
                    this->m_data[1] + v.y(),
                    this->m_data[2] + v.z());
    };

    /// u - v
    Vec3 operator - (const Vec3 v) const {
        return Vec3(this->m_data[0] - v.x(),
                    this->m_data[1] - v.y(),
                    this->m_data[2] - v.z());
    };
};

}

#endif
