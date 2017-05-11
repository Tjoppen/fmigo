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

BaseMaster::BaseMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections) :
        m_clients(clients),
        m_weakConnections(weakConnections),
        clientWeakRefs(getOutputWeakRefs(m_weakConnections)) {
    for(auto client: m_clients)
        client->m_master = this;
}

BaseMaster::~BaseMaster() {
}

#ifdef USE_GPL
int BaseMaster::loop_residual_f(const gsl_vector *x, void *params, gsl_vector *f) {
  fmitcp_master::BaseMaster *master = (fmitcp_master::BaseMaster*)params;
  int k;

  //use initial values, replace reals with x
  //order of elements in x derives from map<FMIClient*, map<fmi2_base_type_enu_t, ...> >
  //this ordering will differ between runs
  InputRefsValuesType newInputs = master->initialNonReals;
  k = 0;
  for (auto& it : newInputs) {
    for (auto& it2 : it.second) {
      if (it2.first == fmi2_base_type_real) {
        for (size_t j = 0; j < it2.second.second.size(); j++, k++) {
          it2.second.second[j].r = gsl_vector_get(x, k);
        }
      }
    }
  }
  if (k != (int)x->size) {
    //this shouldn't happen
    fatal("loop solver ordering logic inconsistent\n");
  }

  //set the x's we got and see what kind of inputs that gives rise to
  //if x is correct then the input values given by getInputWeakRefsAndValues() will be equal to x
  for (auto it : newInputs) {
    it.first->sendSetX(it.second);
  }
  for (auto it : master->clientWeakRefs) {
    it.first->sendGetX(it.second);
  }
  master->wait();

  k = 0;
  //double rtot = 0;
  for (auto it : getInputWeakRefsAndValues(master->m_weakConnections)) {
    for (auto it2 : it.second) {
      if (it2.first == fmi2_base_type_real) {
        std::vector<MultiValue> mvs1 = it2.second.second;
        for (size_t j = 0; j < mvs1.size(); j++, k++) {
          //residual = A*f(x) - x = scaled output - input
          double r = mvs1[j].r - gsl_vector_get(x, k);
          //rtot += r*r;
          //debug("r[%i] = %-.9f - %-.9f = %-.9f\n", k, mvs1[j].r, gsl_vector_get(x, k), r);
          //if (k>0) debug( ",");
          //debug("%-.12f", r);
          gsl_vector_set(f, k, r);
        }
      } else {
        //check that all non-real inputs remain unchanged
        if (it2.second != master->initialNonReals[it.first][it2.first]) {
          fatal("non-real input changed in loop solver. unable to guarantee convergence\n");
        }
      }
    }
  }
  //debug("\nrtot = %.9f\n", rtot);

  return GSL_SUCCESS;
}
#endif

void BaseMaster::solveLoops() {
  //stolen from runIteration()
  for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
    it->first->sendGetX(it->second);
  }
  wait();
  initialNonReals = getInputWeakRefsAndValues(m_weakConnections);

  for (auto it = initialNonReals.begin(); it != initialNonReals.end(); it++) {
    it->first->sendSetX(it->second);
  }

  //count reals
  size_t n = 0;
  for (auto it : initialNonReals) {
    n += it.second[fmi2_base_type_real].second.size();
  }

  if (n == 0) {
    //nothing to do
    return;
  }

  //debug("n=%zu reals\n", n);

#ifdef USE_GPL
  //use GSL if GPL is enabled
  gsl_vector *x0 = gsl_vector_alloc(n);

  //fill x0 with initial reals
  //ordering derives from map<FMIClient*, map<fmi2_base_type_enu_t, ...> >
  int ofs = 0;
  for (auto it : initialNonReals) {
      for (auto multivalue : it.second[fmi2_base_type_real].second) {
          //debug("x0[%i] = %f\n", ofs, multivalue.r);
          gsl_vector_set(x0, ofs++, multivalue.r);
      }
  }

  gsl_multiroot_function f = {loop_residual_f, n, this};
  gsl_multiroot_fsolver *s = gsl_multiroot_fsolver_alloc(gsl_multiroot_fsolver_hybrids, n);

  if (s == NULL) {
    fprintf(stderr, "Failed to allocate multiroot fsolver\n");
    exit(1);
  }

  gsl_multiroot_fsolver_set(s, &f, x0);

  int i = 0, imax = 100, status;
  do {
      if ((status = gsl_multiroot_fsolver_iterate(s)) != 0) {
        break;
      }
      status = gsl_multiroot_test_residual(s->f, 1e-7);
  } while (status == GSL_CONTINUE && ++i < imax);

  //debug("status = %i\n", status);

  if (i >= imax || status != GSL_SUCCESS) {
      fatal("Can't solve the loop, giving up!\n");
  }

  /*debug("solution (%i iterations):\n", i);
  gsl_vector *root = gsl_multiroot_fsolver_root(s);
  for (size_t i = 0; i < root->size; i++) {
     debug("root[%zu] = %f\n", i, gsl_vector_get(root, i));
  }*/

  gsl_multiroot_fsolver_free(s);
  gsl_vector_free(x0);
#else
  fatal("Can't solve algebraic loops without GPL at the moment\n");
#endif
}

size_t BaseMaster::getNumPendingRequests() const {
    size_t ret = 0;
    for (auto s : m_clients) {
        ret += s->getNumPendingRequests();
    }
    return ret;
}

void BaseMaster::wait() {
    //allow polling once for each request, plus 60 seconds more
    int maxPolls = getNumPendingRequests() + 60;
    int numPolls = 0;

    while (getNumPendingRequests() > 0) {
#ifdef USE_MPI
        int rank;
        std::string str = mpi_recv_string(MPI_ANY_SOURCE, &rank, NULL);

        if (rank < 1 || rank > (int)m_clients.size()) {
            fatal("MPI rank out of bounds: %i\n", rank);
        }

        m_clients[rank-1]->Client::clientData(str.c_str(), str.length());
#else
#ifdef WIN32
    //zmq::poll() is broken and incredibly slow on Windows
    //this is the stupidest possible solution, but works surprisingly well
    //it can't detect that a server has croaked however
    for (auto client : m_clients) {
        if (client->getNumPendingRequests() > 0) {
            client->receiveAndHandleMessage();
        }
    }
#else
    //all other platforms (GNU/Linux, Mac)
    //poll all clients, decrease m_pendingRequests as we see REPlies coming in
    vector<zmq::pollitem_t> items(m_clients.size());
    for (size_t x = 0; x < m_clients.size(); x++) {
        items[x].socket = (void*)m_clients[x]->m_socket;
        items[x].events = ZMQ_POLLIN;
    }

#if ZMQ_VERSION_MAJOR == 2
#define ZMQ_POLL_MSEC    1000        //  zmq_poll is usec
#elif ZMQ_VERSION_MAJOR >= 3
#define ZMQ_POLL_MSEC    1           //  zmq_poll is msec
#endif

    int n = zmq::poll(items.data(), m_clients.size(), ZMQ_POLL_MSEC*1000);
    if (!n) {
        debug("polled %li sockets, %li pending (%i/%i), no new events\n", m_clients.size(), getNumPendingRequests(), numPolls, maxPolls);
    }
    for (size_t x = 0; x < m_clients.size(); x++) {
        if (items[x].revents & ZMQ_POLLIN) {
            m_clients[x]->receiveAndHandleMessage();
        }
    }
    if (getNumPendingRequests() > 0) {
        if (++numPolls >= maxPolls) {
            //Jenkins caught something like this, I think
            fatal("Exceeded max number of polls (%i) - stuck?\n", maxPolls);
        }
    } else {
        //debug("wait() done\n");
    }
#endif
#endif
    }
}