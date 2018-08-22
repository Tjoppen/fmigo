/*
 * BaseMaster.h
 *
 *  Created on: Aug 7, 2014
 *      Author: thardin
 */

#ifndef BASEMASTER_H_
#define BASEMASTER_H_

#include "FMIClient.h"
#include <zmq.hpp>
#ifdef USE_GPL
#include <gsl/gsl_multiroots.h>
#include "common/fmigo_storage.h"
using namespace fmigo_storage;
#endif
#include <chrono>

namespace fmitcp_master {
    class BaseMaster {
        int rendezvous;
        //t1 = time at which to perform next step
        std::chrono::high_resolution_clock::time_point t1;
    protected:
        std::vector<FMIClient*> m_clients;
        std::vector<WeakConnection> m_weakConnections;
        OutputRefsType clientWeakRefs;

        InputRefsValuesType initialNonReals;  //for loop solver

#ifdef USE_GPL
        class FmigoStorage m_fmigoStorage;//(std::vector<size_t>(0));
#endif

    public:

        zmq::socket_t rep_socket;
        bool initing, paused, running;
        bool zmqControl;
        int m_pendingRequests;
        double t; //current time

        explicit BaseMaster(zmq::context_t &context, std::vector<FMIClient*> clients, std::vector<WeakConnection> weakConnections);
        virtual ~BaseMaster() {
          info("%i rendezvous\n", rendezvous);
          int messages = 0;
          for(auto client: m_clients)
            messages += client->messages;
          info("%i messages\n", messages);
        }

#ifdef USE_GPL
        static int loop_residual_f(const gsl_vector *x, void *params, gsl_vector *f);

        inline FmigoStorage & get_storage(){return m_fmigoStorage;}
        void storage_alloc(const std::vector<FMIClient*> &clients){
            vector<size_t> states({});
            vector<size_t> indicators({});
            for(auto client: clients) {
                states.push_back(client->getNumContinuousStates());
                indicators.push_back(client->getNumEventIndicators());
            }
            get_storage().allocate_storage(states,indicators);
        }
#endif
      virtual std::string getFieldNames() const {return "";}
      virtual void writeFields(bool last, FILE *outfile) {}
        void solveLoops();
        virtual void prepare() {};
        virtual void runIteration(double t, double dt) = 0;

#define on(name) void name(FMIClient* slave) {}
        on(onSlaveInstantiated)
        on(onSlaveInitialized)
        on(onSlaveTerminated)
        on(onSlaveFreed)
        on(onSlaveStepped)
        on(onSlaveGotVersion)
        on(onSlaveSetReal)
        on(onSlaveGotState)
        on(onSlaveSetState)
        on(onSlaveFreedState)
        on(onSlaveDirectionalDerivative)

        //T is needed because func maybe of a function in Client (from which FMIClient is derived)
        void queueMessage(std::vector<FMIClient*> fmus, std::string str) {
            for (auto it = fmus.begin(); it != fmus.end(); it++) {
                (*it)->queueMessage(str);
            }
        }

        //like queueMessage() but only for one FMU
        void queueMessage(FMIClient *fmu, std::string str) {
            fmu->queueMessage(str);
        }

        //queueMessage() followed by wait() (blocking)
        void sendWait(std::vector<FMIClient*> fmus, std::string str) {
            queueMessage(fmus, str);
            wait();
        }

        //like sendWait() but only for one FMU (blocking)
        void sendWait(FMIClient *fmu, std::string str) {
            queueMessage(fmu, str);
            wait();
        }

        void wait();

        void resetT1();
        //wait for t1, advance t1 by timeStep
        void waitupT1(double timeStep);
        void handleZmqControl();

        void queueValueRequests();
        //if cset_set == true then only delete values for FMUs whose ID is in cset
        void deleteCachedValues(bool cset_set = false, const std::set<int>& cset = std::set<int>());
    };
};

#endif /* BASEMASTER_H_ */
