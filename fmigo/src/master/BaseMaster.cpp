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
#include "serialize.h"

using namespace fmitcp_master;
using namespace fmitcp;

BaseMaster::BaseMaster(zmq::context_t &context, vector<FMIClient*> clients, vector<WeakConnection> weakConnections) :
        rendezvous(0),
        m_clients(clients),
        m_weakConnections(weakConnections),
        clientWeakRefs(getOutputWeakRefs(m_weakConnections)),
        rep_socket(context, ZMQ_REP),
        paused(false),
        running(true),
        zmqControl(false) {
    for(auto client: m_clients)
        client->m_master = this;
}

BaseMaster::~BaseMaster() {
  info("%i rendezvous\n", rendezvous);
  int messages = 0;
  for(auto client: m_clients)
    messages += client->messages;
  info("%i messages\n", messages);
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
  master->deleteCachedValues();
  for (auto it : master->clientWeakRefs) {
    it.first->queueX(it.second);
  }
  master->sendValueRequests();
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
  deleteCachedValues();
  for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
    it->first->queueX(it->second);
  }
  sendValueRequests();
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

    if (getNumPendingRequests() > 0) {
      rendezvous++;
    }

    while (getNumPendingRequests() > 0) {
        handleZmqControl();

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

//converts RepeatedField to std::vector
template<typename T> std::vector<T> rf2vec(const ::google::protobuf::RepeatedField<T>& ts) {
  std::vector<T> ret;
  for (T t : ts) {
    ret.push_back(t);
  }
  return ret;
}

//same but for RepeatedPtrField. works thanks to SFINAE
template<typename T> std::vector<T> rf2vec(const ::google::protobuf::RepeatedPtrField<T>& ts) {
  std::vector<T> ret;
  for (T t : ts) {
    ret.push_back(t);
  }
  return ret;
}

void BaseMaster::handleZmqControl() {
  if (zmqControl) {
    zmq::message_t msg;

    //paused means we do blocking polls to avoid wasting CPU time
    while (rep_socket.recv(&msg, paused ? 0 : ZMQ_NOBLOCK) && running) {
        //got something - make sure it's a control_message with correct
        //version and command set
        control_proto::control_message ctrl;

        if (ctrl.ParseFromArray(msg.data(), msg.size()) &&
            ctrl.has_version() &&
            ctrl.version() == 1) {
          if (ctrl.has_command()) {
            switch (ctrl.command()) {
            case control_proto::control_message::command_pause:
                paused = true;
                break;
            case control_proto::control_message::command_unpause:
                paused = false;
                break;
            case control_proto::control_message::command_stop:
                running = false;
                break;
            case control_proto::control_message::command_state:
                break;
            }
          }

          for (const control_proto::fmu_results& var : ctrl.variables()) {
            if (var.has_fmu_id() && var.fmu_id() >= 0 && (size_t)var.fmu_id() < m_clients.size()) {
              FMIClient *client = m_clients[var.fmu_id()];

#define assertit(x) do { if (!(x)) { fatal("bad variables: !(" #x ")\n"); } } while(0)

              if (var.has_reals()) {
                assertit(var.reals().vrs().size() == var.reals().values().size());
                send(client, serialize::fmi2_import_set_real(rf2vec(var.reals().vrs()), rf2vec(var.reals().values())));
              }
              if (var.has_ints()) {
                assertit(var.ints().vrs().size() == var.ints().values().size());
                send(client, serialize::fmi2_import_set_integer(rf2vec(var.ints().vrs()), rf2vec(var.ints().values())));
              }
              if (var.has_bools()) {
                assertit(var.bools().vrs().size() == var.bools().values().size());
                send(client, serialize::fmi2_import_set_boolean(rf2vec(var.bools().vrs()), rf2vec(var.bools().values())));
              }
              if (var.has_strings()) {
                assertit(var.strings().vrs().size() == var.strings().values().size());
                send(client, serialize::fmi2_import_set_string(rf2vec(var.strings().vrs()), rf2vec<std::string>(var.strings().values())));
              }
            } else {
              warning("bad/unset fmu_id in control_message.variables\n");
            }
          }
        }

            //always reply with state
            control_proto::state_message state;
            state.set_version(1);

            if (!running) {
                state.set_state(control_proto::state_message::state_exiting);
            } else if (paused) {
                state.set_state(control_proto::state_message::state_paused);
            } else {
                state.set_state(control_proto::state_message::state_running);
            }

            for (FMIClient *client : m_clients) {
              control_proto::fmu_state *fstate = state.add_fmu_states();
              fstate->set_fmu_id(client->getId());
              fstate->set_state(client->m_fmuState);
            }

            string str = state.SerializeAsString();
            zmq::message_t rep(str.length());
            memcpy(rep.data(), str.data(), str.length());
            rep_socket.send(rep);
    }
  }
}

void BaseMaster::sendValueRequests() {
  for (FMIClient *client : m_clients) {
    client->sendValueRequests();
  }
}

void BaseMaster::deleteCachedValues() {
  for (FMIClient *client : m_clients) {
    client->deleteCachedValues();
  }
}
