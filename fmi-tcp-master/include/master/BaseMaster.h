/*
 * BaseMaster.h
 *
 *  Created on: Aug 7, 2014
 *      Author: thardin
 */

#ifndef BASEMASTER_H_
#define BASEMASTER_H_

#include "FMIClient.h"

namespace fmitcp_master {
    class BaseMaster {
#ifdef USE_LACEWING
        fmitcp::EventPump *m_pump;
#endif

    protected:
        std::vector<FMIClient*> m_slaves;

    public:
        //number of pending requests sent to clients
        size_t m_pendingRequests;

#ifdef USE_LACEWING
        explicit BaseMaster(fmitcp::EventPump *pump, std::vector<FMIClient*> slaves);
#else
        explicit BaseMaster(std::vector<FMIClient*> slaves);
#endif
        virtual ~BaseMaster();
        virtual void runIteration(double t, double dt) = 0;

        void gotSomething() {
#ifdef USE_LACEWING
            if (m_pendingRequests == 0) {
                fprintf(stderr, "Got response while m_pendingRequests = 0\n");
                exit(1);
            }

            //fprintf(stderr, "m_pendingRequests(%zu)--;\n", m_pendingRequests);
            m_pendingRequests--;
#endif
        }

        // These are callbacks that fire when a slave did something:
        void slaveConnected                 (FMIClient* slave){fprintf(stderr, "Client %i connected\n", slave->getId());}
        void slaveDisconnected              (FMIClient* slave){gotSomething();}
        void slaveError                     (FMIClient* slave){exit(1);}

#define on(name) void name(FMIClient* slave) { /*fprintf(stderr, #name "\n");*/ gotSomething(); }
        on(onSlaveGetXML)
        on(onSlaveInstantiated)
        on(onSlaveInitialized)
        on(onSlaveTerminated)
        on(onSlaveFreed)
        on(onSlaveStepped)
        on(onSlaveGotVersion)
        on(onSlaveSetReal)
        on(onSlaveGotReal)
        on(onSlaveGotState)
        on(onSlaveSetState)
        on(onSlaveFreedState)
        on(onSlaveDirectionalDerivative)

        //T is needed because func maybe of a function in Client (from which FMIClient is derived)
        void send(std::vector<FMIClient*> fmus, std::string str) {
            m_pendingRequests += fmus.size();
            for (auto it = fmus.begin(); it != fmus.end(); it++) {
                (*it)->sendMessage(str);
            }
        }

        //like send() but only for one FMU
        void send(FMIClient *fmu, std::string str) {
            m_pendingRequests++;
            fmu->sendMessage(str);
        }

        //like send() except it blocks
        void block(std::vector<FMIClient*> fmus, std::string str) {
            send(fmus, str);
            wait();
        }

        //like block() but only for one FMU
        void block(FMIClient *fmu, std::string str) {
            send(fmu, str);
            wait();
        }

        void wait();
    };
};

#endif /* BASEMASTER_H_ */
