#ifndef SLAVE_H_
#define SLAVE_H_

#include "lacewing.h"

namespace fmitcp_master {

    /// Container for slave data from the master's perspective.
    class Slave {

    public:

        enum SlaveState {
            SLAVE_NONE,
            SLAVE_INITIALIZED,
            SLAVE_STEPPING,
            SLAVE_STEPPINGFINISHED
        };

        Slave(lw_client);
        ~Slave();

        void doStep();
        lw_client getClient();
        int getId();
        void setId(int id);
        void setState(SlaveState s);
        SlaveState getState();
        void initialize(double relativeTolerance, double tStart, bool stopTimeDefined, double tStop);
        void instantiate();
        void setInitialValues();
        void terminate();
        void getReal(int valueRef);
        void setReal(int valueRef, double value);
        bool isConnected();

    private:
        SlaveState m_state;
        lw_client m_client;
        int m_id;
    };
};

#endif
