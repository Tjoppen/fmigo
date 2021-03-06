/*
 * BaseMaster.cpp
 *
 *  Created on: Aug 7, 2014
 *      Author: thardin
 */

#include "master/globals.h"
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
        initing(true),
        paused(false),
        running(false),
        zmqControl(false),
        m_pendingRequests(0),
        t(0.0) {
    for(auto client: m_clients)
        client->m_master = this;
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
    for (size_t j = 0; j < it.second.reals.size(); j++, k++) {
      it.second.reals[j] = gsl_vector_get(x, k);
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
  master->queueValueRequests();
  master->wait();

  k = 0;
  //double rtot = 0;
  for (auto it : getInputWeakRefsAndValues(master->m_weakConnections)) {
        for (size_t j = 0; j < it.second.reals.size(); j++, k++) {
          //residual = A*f(x) - x = scaled output - input
          double r = it.second.reals[j] - gsl_vector_get(x, k);
          //rtot += r*r;
          //debug("r[%i] = %-.9f - %-.9f = %-.9f\n", k, mvs1[j].r, gsl_vector_get(x, k), r);
          //if (k>0) debug( ",");
          //debug("%-.12f", r);
          gsl_vector_set(f, k, r);
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
  queueValueRequests();
  wait();
  initialNonReals = getInputWeakRefsAndValues(m_weakConnections);

  for (auto it = initialNonReals.begin(); it != initialNonReals.end(); it++) {
    it->first->sendSetX(it->second);
  }

  //count reals
  size_t n = 0;
  for (auto it : initialNonReals) {
    n += it.second.reals.size();
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
      for (double r : it.second.reals) {
          //debug("x0[%i] = %f\n", ofs, r);
          gsl_vector_set(x0, ofs++, r);
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

void BaseMaster::wait() {
    //allow polling once for each request, plus 60 seconds more
    int maxPolls = m_pendingRequests + 60;
    int numPolls = 0;

    if (m_pendingRequests > 0) {
      rendezvous++;

      //send messages
      for (FMIClient *client : m_clients) {
        client->sendQueuedMessages();
      }
    }

    while (m_pendingRequests > 0) {
        handleZmqControl();
        fmigo::globals::timer.rotate("pre_wait");

#ifdef USE_MPI
        int rank;
        mpi_recv_string(MPI_ANY_SOURCE, &rank, NULL, m_mpi_str);
        fmigo::globals::timer.rotate("wait");

        if (rank < 1 || rank > (int)m_clients.size()) {
            fatal("MPI rank out of bounds: %i\n", rank);
        }

        m_clients[rank-1]->Client::clientData(m_mpi_str.c_str(), m_mpi_str.length());
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
    fmigo::globals::timer.rotate("wait");
    if (!n) {
        debug("polled %li sockets, %i pending (%i/%i), no new events\n", m_clients.size(), m_pendingRequests, numPolls, maxPolls);
    }
    for (size_t x = 0; x < m_clients.size(); x++) {
        if (items[x].revents & ZMQ_POLLIN) {
            m_clients[x]->receiveAndHandleMessage();
        }
    }
    if (m_pendingRequests > 0) {
        if (++numPolls >= maxPolls) {
            //Jenkins caught something like this, I think
            fatal("Exceeded max number of polls (%i) - stuck?\n", maxPolls);
        }
    } else {
        //debug("wait() done\n");
    }
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

void BaseMaster::resetT1() {
  t1 = std::chrono::high_resolution_clock::now();
}

void BaseMaster::waitupT1(double timeStep) {
  //delay loop
  for (;;) {
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    if (t2 >= t1) {
      break;
    }
#ifdef WIN32
    Yield();
#else
    usleep(std::chrono::duration<double, std::micro>(t1 - t2).count());
#endif
  }

  //aren't C++ templates wonderful?
  t1 += std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::duration<double>(timeStep));
}

void BaseMaster::handleZmqControl() {
  if (zmqControl) {
    zmq::message_t msg;

    //paused means we do blocking polls to avoid wasting CPU time
    while (rep_socket.recv(&msg, paused ? 0 : ZMQ_NOBLOCK) && (initing || running)) {
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
                if (paused) {
                  //step immediately after unpause
                  resetT1();
                }
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
                queueMessage(client, serialize::fmi2_import_set_real(rf2vec(var.reals().vrs()), rf2vec(var.reals().values())));
              }
              if (var.has_ints()) {
                assertit(var.ints().vrs().size() == var.ints().values().size());
                queueMessage(client, serialize::fmi2_import_set_integer(rf2vec(var.ints().vrs()), rf2vec(var.ints().values())));
              }
              if (var.has_bools()) {
                assertit(var.bools().vrs().size() == var.bools().values().size());
                queueMessage(client, serialize::fmi2_import_set_boolean(rf2vec(var.bools().vrs()), rf2vec(var.bools().values())));
              }
              if (var.has_strings()) {
                assertit(var.strings().vrs().size() == var.strings().values().size());
                queueMessage(client, serialize::fmi2_import_set_string(rf2vec(var.strings().vrs()), rf2vec<std::string>(var.strings().values())));
              }
            } else {
              warning("bad/unset fmu_id in control_message.variables\n");
            }
          }
        }

            //always reply with state
            control_proto::state_message state;
            state.set_version(1);
            state.set_t(t);

            if (initing) {
                state.set_state(control_proto::state_message::state_initing);
            } else if (!running) {
                state.set_state(control_proto::state_message::state_exiting);
            } else if (paused) {
                state.set_state(control_proto::state_message::state_paused);
            } else {
                state.set_state(control_proto::state_message::state_running);
            }

            for (FMIClient *client : m_clients) {
              control_proto::fmu_state *fstate = state.add_fmu_states();
              fstate->set_fmu_id(client->m_id);
              fstate->set_state(client->m_fmuState);
            }

            string str = state.SerializeAsString();
            zmq::message_t rep(str.length());
            memcpy(rep.data(), str.data(), str.length());
            rep_socket.send(rep);
    }
  }
}

void BaseMaster::queueValueRequests() {
  for (FMIClient *client : m_clients) {
    client->queueValueRequests();
  }
}

void BaseMaster::deleteCachedValues(bool cset_set, const fmitcp::int_set& cset) {
  for (FMIClient *client : m_clients) {
    if (!cset_set || cset.count(client->m_id)) {
      client->deleteCachedValues();
    }
  }
}
