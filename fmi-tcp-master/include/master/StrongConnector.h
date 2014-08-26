#ifndef MASTER_STRONGCONNECTOR_H_
#define MASTER_STRONGCONNECTOR_H_

#include <sc/Connector.h>

namespace fmitcp_master {

    class FMIClient;

    /**
     * @brief Container for value references, that specify a mechanical connector in a simulation slave.
     * @todo Should really use some kind of vector class
     */
    class StrongConnector {

    protected:

        /// The owner FMIClient
        FMIClient* m_client;

        /// Corresponding sc connector
        sc::Connector m_conn;

        bool m_hasPosition;
        int  m_vref_position[3];

        bool m_hasQuaternion;
        int  m_vref_quaternion[4];

        bool m_hasVelocity;
        int  m_vref_velocity[3];

        bool m_hasAngularVelocity;
        int  m_vref_angularVelocity[3];

        bool m_hasForce;
        int  m_vref_force[3];

        bool m_hasTorque;
        int  m_vref_torque[3];

    public:
        StrongConnector(FMIClient* slave);
        virtual ~StrongConnector();

        FMIClient * getSlave();
        sc::Connector * getConnector();

        /// Set the value references of the positions
        void setPositionValueRefs(int,int,int);
        void setQuaternionValueRefs(int,int,int,int);
        void setVelocityValueRefs(int,int,int);
        void setAngularVelocityValueRefs(int,int,int);
        void setForceValueRefs(int,int,int);
        void setTorqueValueRefs(int,int,int);

        /// Indicates if the positional valuereferences have been set
        bool hasPosition();
        bool hasQuaternion();
        bool hasVelocity();
        bool hasAngularVelocity();
        bool hasForce();
        bool hasTorque();

        /// Get value references of the 3 position values
        std::vector<int> getPositionValueRefs() const;
        std::vector<int> getQuaternionValueRefs() const;
        std::vector<int> getVelocityValueRefs() const;
        std::vector<int> getAngularVelocityValueRefs() const;
        std::vector<int> getForceValueRefs() const;
        std::vector<int> getTorqueValueRefs() const;

        /// Set all connector values, given value references and values
        void setValues(std::vector<int> valueReferences, std::vector<double> values);

        /// Set velocities of bodies one step forward
        void setFutureValues(std::vector<int> valueReferences, std::vector<double> values);
    };
};

#endif
