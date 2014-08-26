#include "master/Connection.h"

using namespace fmitcp_master;

Connection::Connection( FMIClient * slaveA,
                        FMIClient * slaveB ){
    m_slaveA = slaveA;
    m_slaveB = slaveB;
}
Connection::~Connection(){

}

ConnectionState Connection::getState(){
    return m_state;
}

void Connection::setState(ConnectionState s){
    s = m_state;
}

FMIClient * Connection::getSlaveA(){
    return m_slaveA;
}

FMIClient * Connection::getSlaveB(){
    return m_slaveB;
}

