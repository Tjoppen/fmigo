#include "master/Slave.h"
#include "lacewing.h"
#include "string.h"

using namespace fmitcp_master;

Slave::Slave(lw_client client){
    m_client = client;
    m_id = 0;
}

Slave::~Slave(){

}

void Slave::doStep(){

}

lw_client Slave::getClient(){
    return m_client;
}

int Slave::getId(){
    return m_id;
}

void Slave::setId(int id){
    m_id = id;
}

void Slave::initialize(double relativeTolerance, double tStart, bool stopTimeDefined, double tStop){
    /*
        char cmd[50];

    sprintf(cmd, "%s%f", Message::fmiTStart.c_str(), tStart);
    sendCommand(cmd,strlen(cmd));

    sprintf(cmd, "%s%f", Message::fmiStepSize.c_str(),   0.1);
    sendCommand(cmd,strlen(cmd));

    sprintf(cmd, "%s%f", Message::fmiTEnd.c_str(),       tStop);
    sendCommand(cmd,strlen(cmd));
    */
}

void Slave::instantiate(){

}

void Slave::setInitialValues(){

}

void Slave::terminate(){

}

void Slave::setState(SlaveState s){
    m_state = s;
}

Slave::SlaveState Slave::getState(){
    return m_state;
}

void Slave::getReal(int valueRef){

}

void Slave::setReal(int valueRef, double value){

}

bool Slave::isConnected(){
    return lw_client_connected(m_client);
}
