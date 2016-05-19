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
    protected:
        std::vector<FMIClient*> m_clients;

    public:
        //number of pending requests sent to clients
        size_t getNumPendingRequests() const;

        explicit BaseMaster(std::vector<FMIClient*> clients);
        virtual ~BaseMaster();
        virtual void prepare() {};
        virtual void runIteration(double t, double dt) = 0;

        // These are callbacks that fire when a slave did something:
        void slaveConnected                 (FMIClient* slave){fprintf(stderr, "Client %i connected\n", slave->getId());}
        void slaveDisconnected              (FMIClient* slave){}
        void slaveError                     (FMIClient* slave){exit(1);}

#define on(name) void name(FMIClient* slave) {}
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
            for (auto it = fmus.begin(); it != fmus.end(); it++) {
                (*it)->sendMessage(str);
            }
        }

        //like send() but only for one FMU
        void send(FMIClient *fmu, std::string str) {
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
