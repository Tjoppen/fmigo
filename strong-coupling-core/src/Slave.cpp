#include "sc/Slave.h"
#include "sc/Connector.h"

using namespace sc;

Slave::Slave(){}
Slave::~Slave() {
    for (Connector* conn : m_connectors) {
        delete conn;
    }
}

void Slave::addConnector(Connector * conn){
    m_connectors.push_back(conn);
    conn->m_slave = this;
}

int Slave::numConnectors(){
    return m_connectors.size();
}

Connector * Slave::getConnector(int i){
    return m_connectors[i];
}

