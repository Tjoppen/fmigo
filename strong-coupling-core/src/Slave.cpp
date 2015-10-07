#include "sc/Slave.h"
#include "sc/Connector.h"

using namespace sc;

Slave::Slave(){}
Slave::~Slave(){}

void Slave::addConnector(Connector * conn){
    m_connectors.push_back(conn);
    conn->m_slave = (void*)this;
}

int Slave::numConnectors(){
    return m_connectors.size();
}

Connector * Slave::getConnector(int i){
    return m_connectors[i];
}

