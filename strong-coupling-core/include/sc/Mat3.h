#ifndef SCMAT3_H
#define SCMAT3_H

#include "stdlib.h"
#include "sc/Vec3.h"
#include "sc/Quat.h"

namespace sc {

    /// 3x3 Matrix class
    class Mat3 {

        private:
            /**
             * @brief data storage
             *
             * Layout:
             * 0 1 2
             * 3 4 5
             * 6 7 8
             */
            double m_data[9];

        public:
            Mat3();
            Mat3(const Quat& q);
            virtual ~Mat3();

            void set(double, double, double,
                     double, double, double,
                     double, double, double);
            Mat3 multiplyMatrix(const Mat3& m) const;
            Vec3 multiplyVector(const Vec3& m) const;
            void setRotationFromQuaternion(const Quat& q);

            /// Scale each column in the matrix
            Mat3 scale(const Vec3& v);

            /// Get a column as a vector
            Vec3 getColumn(int i) const;

            double getElement(int row, int column) const;
            void setElement(int row, int column, double value);
            Mat3 transpose() const;

            /// Copy another matrix
            void copy(const Mat3& m);

            /// A*B
            Mat3 operator* (const Mat3& y) const {
                return this->multiplyMatrix(y);
            }

            /// A*x
            Vec3 operator* (const Vec3& y) const {
                return this->multiplyVector(y);
            }

    };

};

#endif
