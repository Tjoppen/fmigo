#include "master/WeakConnection.h"
#include "master/Master.h"

using namespace fmitcp_master;

WeakConnection::WeakConnection( FMIClient* slaveA,
                                FMIClient* slaveB,
                                int valueRefA,
                                int valueRefB ) : Connection(slaveA, slaveB){
    m_valueRefA = valueRefA;
    m_valueRefB = valueRefB;
}
WeakConnection::~WeakConnection(){

}

int WeakConnection::getValueRefA(){
    return m_valueRefA;
}

int WeakConnection::getValueRefB(){
    return m_valueRefB;
}
