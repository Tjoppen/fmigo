#ifndef CONNECTION_H_
#define CONNECTION_H_

#include "master/FMIClient.h"

namespace fmitcp_master {

    enum ConnectionState {
      CONNECTION_INVALID,
      CONNECTION_REQUESTED,
      CONNECTION_COMPLETE
    };

    /// Base class for connections
    class Connection {

    protected:
        FMIClient* m_slaveA;
        FMIClient* m_slaveB;
        ConnectionState m_state;

    public:
        Connection(FMIClient* slaveA, FMIClient* slaveB);
        virtual ~Connection();

        ConnectionState getState();
        void setState(ConnectionState s);

        FMIClient * getSlaveA();
        FMIClient * getSlaveB();
    };
};

#endif
