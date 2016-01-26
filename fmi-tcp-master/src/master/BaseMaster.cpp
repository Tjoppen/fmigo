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
#ifdef USE_MPI
#include "common/mpi_tools.h"
#endif

using namespace fmitcp_master;
using namespace fmitcp;

BaseMaster::BaseMaster(vector<FMIClient*> slaves) :
        m_slaves(slaves) {
}

BaseMaster::~BaseMaster() {
}

size_t BaseMaster::getNumPendingRequests() const {
    size_t ret = 0;
    for (auto s : m_slaves) {
        ret += s->getNumPendingRequests();
    }
    return ret;
}

void BaseMaster::wait() {
    //allow polling once for each request, plus ten seconds more
    int maxPolls = getNumPendingRequests() + 10;
    int numPolls = 0;

    while (getNumPendingRequests() > 0) {
#ifdef USE_MPI
        int rank;
        std::string str = mpi_recv_string(MPI_ANY_SOURCE, &rank, NULL);

        if (rank < 1 || rank > m_slaves.size()) {
            fprintf(stderr, "MPI rank out of bounds: %i\n", rank);
            exit(1);
        }

        m_slaves[rank-1]->Client::clientData(str.c_str(), str.length());
#else
    //poll all clients, decrease m_pendingRequests as we see REPlies coming in
    vector<zmq::pollitem_t> items(m_slaves.size());
    for (size_t x = 0; x < m_slaves.size(); x++) {
        items[x].socket = (void*)m_slaves[x]->m_socket;
        items[x].events = ZMQ_POLLIN;
    }
    int n = zmq::poll(items.data(), m_slaves.size(), 1000000);
    if (!n) {
        fprintf(stderr, "polled %li sockets, %li pending (%i/%i), no new events\n", m_slaves.size(), getNumPendingRequests(), numPolls, maxPolls);
    }
    for (size_t x = 0; x < m_slaves.size(); x++) {
        if (items[x].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            m_slaves[x]->m_socket.recv(&msg);
            //fprintf(stderr, "Got message of size %li\n", msg.size());
            m_slaves[x]->Client::clientData(static_cast<char*>(msg.data()), msg.size());
        }
    }
    if (getNumPendingRequests() > 0) {
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
