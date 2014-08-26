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
        fmitcp::EventPump *m_pump;

    protected:
        std::vector<FMIClient*> m_slaves;

    public:
        //number of pending requests sent to clients
        size_t m_pendingRequests;

        explicit BaseMaster(fmitcp::EventPump *pump, std::vector<FMIClient*> slaves);
        virtual ~BaseMaster();
        virtual void runIteration(double t, double dt) = 0;

        void gotSomething() {
            if (m_pendingRequests == 0) {
                fprintf(stderr, "Got response while m_pendingRequests = 0\n");
                exit(1);
            }

            //fprintf(stderr, "m_pendingRequests(%zu)--;\n", m_pendingRequests);
            m_pendingRequests--;
        }

        // These are callbacks that fire when a slave did something:
        void slaveConnected                 (FMIClient* slave){fprintf(stderr, "Client %i connected\n", slave->getId()); gotSomething();}
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
        template<typename T, typename... Params, typename... FnParams> void send(std::vector<FMIClient*> fmus, void (T::*func)(FnParams...), Params&&... args) {
            m_pendingRequests += fmus.size();
            for (auto it = fmus.begin(); it != fmus.end(); it++) {
                ((*it)->*func)(std::forward<Params>(args)...);
            }
        }

        //like send() but only for one FMU
        template<typename T, typename... Params, typename... FnParams> void send(FMIClient *fmu, void (T::*func)(FnParams...), Params&&... args) {
            m_pendingRequests++;
            (fmu->*func)(std::forward<Params>(args)...);
        }

        //like send() except it blocks
        template<typename T, typename... Params, typename... FnParams> void block(std::vector<FMIClient*> fmus, void (T::*func)(FnParams...), Params&&... args) {
            send(fmus, func, std::forward<Params>(args)...);
            wait();
        }

        //like block() but only for one FMU
        template<typename T, typename... Params, typename... FnParams> void block(FMIClient *fmu, void (T::*func)(FnParams...), Params&&... args) {
            send(fmu, func, std::forward<Params>(args)...);
            wait();
        }

        void wait();
    };
};

#endif /* BASEMASTER_H_ */
