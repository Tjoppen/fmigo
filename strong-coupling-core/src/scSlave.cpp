#include "scSlave.h"
#include "scConnector.h"

scSlave::scSlave(){
}
scSlave::~scSlave(){}

void scSlave::addConnector(scConnector * conn){
    m_connectors.push_back(conn);
}

int scSlave::numConnectors(){
    return m_connectors.size();
}

scConnector * scSlave::getConnector(int i){
    return m_connectors[i];
}
