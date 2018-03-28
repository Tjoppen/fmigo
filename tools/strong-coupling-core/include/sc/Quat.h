#ifndef SCQUAT_H
#define SCQUAT_H

#include "stdlib.h"
#include "sc/Vec3.h"

namespace sc {

/// Quaternion class
class Quat {
private:
    /// Quaternion data: [x,y,z,w]
    double m_data[4];

public:
    Quat();
    Quat(double x, double y, double z, double w);
    virtual ~Quat();

    /// \return a reference to the i:th element of the quaternion in the order x,y,z,w
    double& operator [] ( int i ){
        return m_data[i];
    };

    /// \return the value of the i:th element of the quaternion in the order x,y,z,w
    double  operator [] ( int i ) const {
        return m_data[i];
    };

    /// Multiply with other quaternion
    Quat multiply(const Quat& p) const;

    /// Transform vector
    Vec3 multiplyVector(const Vec3& v) const;
    Vec3 getAxis() const;

    /// Normalize this quat. Will change state.
    void normalize();

    /// Set all values
    void set(double x, double y, double z, double w);

    /// Copy state from other quaternion
    void copy(const Quat& q);

    double x() const;
    double y() const;
    double z() const;
    double w() const;
};

}

#endif
