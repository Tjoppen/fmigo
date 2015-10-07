#ifndef SCSLAVE_H
#define SCSLAVE_H

#include "scConnector.h"
#include <vector>

class scSlave {

private:
    std::vector<scConnector*> m_connectors;

public:
    scSlave();
    ~scSlave();

    /// Arbitrary user data
    void * userData;

    /// Add a connector to the slave.
    void addConnector(scConnector * connector);

    // Get total number of connectors attached.
    int numConnectors();

    /// Get one of the connectors
    scConnector * getConnector(int i);
};

#endif
