#ifndef SCJACOBIANELEMENT_H
#define SCJACOBIANELEMENT_H

#include "sc/Vec3.h"

namespace sc {

    /// An element containing 6 entries, 3 spatial and 3 rotational degrees of freedom.
    class JacobianElement {

        private:
            Vec3 m_spatial;
            Vec3 m_rotational;

        public:

            JacobianElement();
            virtual ~JacobianElement();

            double multiply(const Vec3& spatial, const Vec3& rotational);
            double multiply(const JacobianElement& e);

            void setSpatial(double,double,double);
            void setRotational(double,double,double);
            void setSpatial(const Vec3&);
            void setRotational(const Vec3&);

            Vec3 getSpatial() const;
            Vec3 getRotational() const;
            void print();
    };

};

#endif
