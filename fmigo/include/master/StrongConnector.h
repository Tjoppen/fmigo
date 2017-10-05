#ifndef MASTER_STRONGCONNECTOR_H_
#define MASTER_STRONGCONNECTOR_H_

#include <sc/Connector.h>

namespace fmitcp_master {

    class FMIClient;

    /**
     * @brief Container for value references, that specify a mechanical connector in a simulation slave.
     * @todo Should really use some kind of vector class
     */
    class StrongConnector : public sc::Connector {

    protected:
        bool m_hasPosition;
        int  m_vref_position[3];

        bool m_hasQuaternion;
        int  m_vref_quaternion[4];

        bool m_hasShaftAngle;
        int  m_vref_shaftAngle;

        bool m_hasVelocity;
        int  m_vref_velocity[3];

        bool m_hasAcceleration;
        int  m_vref_acceleration[3];

        bool m_hasAngularVelocity;
        int  m_vref_angularVelocity[3];

        bool m_hasAngularAcceleration;
        int  m_vref_angularAcceleration[3];

        bool m_hasForce;
        int  m_vref_force[3];

        bool m_hasTorque;
        int  m_vref_torque[3];

        mutable std::vector<int> m_PositionValueRefs;
        mutable std::vector<int> m_QuaternionValueRefs;
        mutable std::vector<int> m_ShaftAngleValueRefs;
        mutable std::vector<int> m_VelocityValueRefs;
        mutable std::vector<int> m_AccelerationValueRefs;
        mutable std::vector<int> m_AngularVelocityValueRefs;
        mutable std::vector<int> m_AngularAccelerationValueRefs;
        mutable std::vector<int> m_ForceValueRefs;
        mutable std::vector<int> m_TorqueValueRefs;

        mutable bool m_hasComputedPositionValueRefs;
        mutable bool m_hasComputedQuaternionValueRefs;
        mutable bool m_hasComputedShaftAngleValueRefs;
        mutable bool m_hasComputedVelocityValueRefs;
        mutable bool m_hasComputedAccelerationValueRefs;
        mutable bool m_hasComputedAngularVelocityValueRefs;
        mutable bool m_hasComputedAngularAccelerationValueRefs;
        mutable bool m_hasComputedForceValueRefs;
        mutable bool m_hasComputedTorqueValueRefs;

    public:
        StrongConnector(FMIClient* slave);
        virtual ~StrongConnector();

        /// Set the value references of the positions
        void setPositionValueRefs(int,int,int);
        void setQuaternionValueRefs(int,int,int,int);
        void setShaftAngleValueRef(int);
        void setVelocityValueRefs(int,int,int);
        void setAccelerationValueRefs(int,int,int);
        void setAngularVelocityValueRefs(int,int,int);
        void setAngularAccelerationValueRefs(int,int,int);
        void setForceValueRefs(int,int,int);
        void setTorqueValueRefs(int,int,int);

        /// Indicates if the positional valuereferences have been set
        bool hasPosition();
        bool hasQuaternion();
        bool hasShaftAngle();
        bool hasVelocity();
        bool hasAcceleration();
        bool hasAngularVelocity();
        bool hasAngularAcceleration();
        bool hasForce();
        bool hasTorque();

        /// Get value references of the 3 position values
        const std::vector<int>& getPositionValueRefs() const;
        const std::vector<int>& getQuaternionValueRefs() const;
        const std::vector<int>& getShaftAngleValueRefs() const;
        const std::vector<int>& getVelocityValueRefs() const;
        const std::vector<int>& getAccelerationValueRefs() const;
        const std::vector<int>& getAngularVelocityValueRefs() const;
        const std::vector<int>& getAngularAccelerationValueRefs() const;
        const std::vector<int>& getForceValueRefs() const;
        const std::vector<int>& getTorqueValueRefs() const;

        /// Set all connector values, given value references and values
        void setValues(std::vector<int> valueReferences, std::vector<double> values);

        /// Set velocities of bodies one step forward
        void setFutureValues(std::vector<int> valueReferences, std::vector<double> values);

        bool matchesBallLockConnector(
                int posX, int posY, int posZ,
                int accX, int accY, int accZ,
                int forceX, int forceY, int forceZ,
                int quatX, int quatY, int quatZ, int quatW,
                int angAccX, int angAccY, int angAccZ,
                int torqueX, int torqueY, int torqueZ);
        bool matchesShaftConnector(int angle, int angularVel, int angularAcc, int torque);
    };
};

#endif
