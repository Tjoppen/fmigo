#ifndef SCSLAVE_H
#define SCSLAVE_H

#include "sc/Connector.h"
#include <vector>

namespace sc {

/// Represents a subsystem and holds connectors.
class Slave {

private:
    std::vector<Connector*> m_connectors;

public:
    Slave();
    virtual ~Slave();

    /// Arbitrary user data
    void * userData;

    /// Add a connector to the slave.
    void addConnector(Connector * connector);

    // Get total number of connectors attached.
    int numConnectors();

    /// Get one of the connectors
    Connector * getConnector(int i);
};

}

#endif
