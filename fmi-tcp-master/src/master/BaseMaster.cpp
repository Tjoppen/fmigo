/*
 * BaseMaster.cpp
 *
 *  Created on: Aug 7, 2014
 *      Author: thardin
 */

#include "master/BaseMaster.h"
#include <unistd.h>

using namespace fmitcp_master;
using namespace fmitcp;

BaseMaster::BaseMaster(EventPump *pump, vector<FMIClient*> slaves) :
        m_pendingRequests(0),
        m_pump(pump),
        m_slaves(slaves) {
}

BaseMaster::~BaseMaster() {
}

void BaseMaster::wait() {
    while (m_pendingRequests > 0) {
        m_pump->tick();
        usleep(10);
    }
}
