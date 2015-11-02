/*
 * BaseMaster.cpp
 *
 *  Created on: Aug 7, 2014
 *      Author: thardin
 */

#include "master/BaseMaster.h"
#ifndef WIN32
#include <unistd.h>
#endif

using namespace fmitcp_master;
using namespace fmitcp;

#ifdef USE_LACEWING
BaseMaster::BaseMaster(EventPump *pump, vector<FMIClient*> slaves) :
        m_pendingRequests(0),
        m_pump(pump),
#else
BaseMaster::BaseMaster(vector<FMIClient*> slaves) :
        m_pendingRequests(0),
#endif
        m_slaves(slaves) {
}

BaseMaster::~BaseMaster() {
}

void BaseMaster::wait() {
    //allow polling once for each request, plus ten seconds more
    int maxPolls = m_pendingRequests + 10;
    int numPolls = 0;

    while (m_pendingRequests > 0) {
#ifdef USE_LACEWING
        m_pump->tick();
#ifdef WIN32
        Yield();
#else
        usleep(10);
#endif
#else
    //poll all clients, decrease m_pendingRequests as we see REPlies coming in
    vector<zmq::pollitem_t> items(m_slaves.size());
    for (size_t x = 0; x < m_slaves.size(); x++) {
        items[x].socket = m_slaves[x]->m_socket;
        items[x].events = ZMQ_POLLIN;
    }
    int n = zmq::poll(items.data(), m_slaves.size(), 1000000);
    if (!n) {
        fprintf(stderr, "polled %li sockets, %li pending (%i/%i), no new events\n", m_slaves.size(), m_pendingRequests, numPolls, maxPolls);
    }
    for (size_t x = 0; x < m_slaves.size(); x++) {
        if (items[x].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            m_slaves[x]->m_socket.recv(&msg);
            //fprintf(stderr, "Got message of size %li\n", msg.size());
            m_pendingRequests--;
            m_slaves[x]->Client::clientData(static_cast<char*>(msg.data()), msg.size());
        }
    }
    if (m_pendingRequests > 0) {
        if (++numPolls >= maxPolls) {
            //Jenkins caught something like this, I think
            fprintf(stderr, "Exceeded max number of polls (%i) - stuck?\n", maxPolls);
            exit(1);
        }
    } else {
        //fprintf(stderr, "wait() done\n");
    }
#endif
    }
}
