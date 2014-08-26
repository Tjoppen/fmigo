#ifndef MASTER_H_
#define MASTER_H_

#include <vector>
#include "lacewing.h"
#include <limits.h>
#include <string>
#include <fmitcp/EventPump.h>
#include <fmitcp/Logger.h>
#include <sc/Solver.h>

#include "master/WeakConnection.h"

namespace fmitcp_master {

    class FMIClient;

    enum WeakCouplingAlgorithm {
        SERIAL,
        PARALLEL
    };

    /**
     * The master state. Basically, the simulation loop looks like this:
     *
     * Start everything up:
     * 1. instantiate
     * 2. initialize
     *
     * Simulation loop runs until we reach end time:
     * 3. simulation loop
     *
     *     3.1 If there are strong coupling connections:
     *
     *         We need velocities one step ahead for the strong coupling:
     *         3.1.1 getState
     *         3.1.2 doStep
     *         3.1.3 getReal       (get only velocities)
     *         3.1.4 setState      (rewind)
     *
     *         And also directional derivatives:
     *         3.1.5 getStrongCouplingConnectorStates
     *         3.1.6 getDirecionalDerivatives
     *
     *         The resulting strong coupling constraint forces are applied:
     *         3.1.7 setReal       (strong coupling forces)
     *
     *     We transfer values from weak coupling:
     *     3.2 setReal
     *
     *     Final step.
     *     3.3 doStep
     */
    enum MasterState {
        MASTER_STATE_START,
        MASTER_STATE_CONNECTING_SLAVES,
        MASTER_STATE_START_SIMLOOP,
        MASTER_STATE_GETTING_VERSION,
        MASTER_STATE_GETTING_XML,
        MASTER_STATE_INSTANTIATING_SLAVES,
        MASTER_STATE_INITIALIZING_SLAVES,
        MASTER_STATE_TRANSFERRING_WEAK,
        MASTER_STATE_GETTING_STATES,
        MASTER_STATE_STEPPING_SLAVES_FOR_FUTURE_VELO,
        MASTER_STATE_GETTING_FUTURE_VELO,
        MASTER_STATE_SETTING_STATES,
        MASTER_STATE_GETTING_DIRECTIONAL_DERIVATIVES,
        MASTER_STATE_SETTING_STRONG_COUPLING_FORCES,
        MASTER_STATE_STEPPING_SLAVES,
        MASTER_STATE_GET_WEAK_REALS,
        MASTER_STATE_SET_WEAK_REALS,
        MASTER_STATE_GET_STRONG_CONNECTOR_STATES,
        MASTER_STATE_GET_OUTPUTS,
        MASTER_STATE_DONE
    };

    /**
     * @brief Handles all connections to slaves connected via TCP and requests them to, for example, step.
     * The master provides several stepping algorithms and methods for changing settings regarding these.
     */
    class Master {

    private:

        /// All weak connections defined
        std::vector<WeakConnection*> m_weakConnections;

        /// All connected slaves
        std::vector<FMIClient*> m_slaves;

        /// Is this used ?
        std::vector<int> m_slave_ids;

        fmitcp::EventPump * m_pump;
        int m_slaveIdCounter;
        fmitcp::Logger m_logger;
        WeakCouplingAlgorithm m_method;

        /// Current master state
        MasterState m_state;
        double m_relativeTolerance;
        double m_timeStep;
        double m_startTime;
        double m_endTime;
        bool m_endTimeDefined;

        /// Current time
        double m_time;

        /// Solver that solves strong connections
        sc::Solver m_strongCouplingSolver;

        /// Strong coupling slaves for the sc library.
        std::vector<sc::Slave*> m_strongCouplingSlaves;

        /// sc lib connectors
        std::vector<sc::Connector*> m_strongCouplingConnectors;

    public:

        // Create a master instance with the given logger and event pump
        Master(const fmitcp::Logger& logger, fmitcp::EventPump* pump);
        virtual ~Master();

        void init();

        fmitcp::EventPump * getEventPump();
        fmitcp::Logger * getLogger();

        /// Connects to a slave and gets info about it
        FMIClient* connectSlave(std::string uri);

        /// Get a slave by id
        FMIClient * getSlave(int id);

        /// Set the master state
        void setState(MasterState state);

        // These are callbacks that fire when a slave did something:
        void slaveConnected                 (FMIClient* slave);
        void slaveDisconnected              (FMIClient* slave);
        void slaveError                     (FMIClient* slave);
        void onSlaveGetXML                  (FMIClient* slave);
        void onSlaveInstantiated            (FMIClient* slave);
        void onSlaveInitialized             (FMIClient* slave);
        void onSlaveTerminated              (FMIClient* slave);
        void onSlaveFreed                   (FMIClient* slave);
        void onSlaveStepped                 (FMIClient* slave);
        void onSlaveGotVersion              (FMIClient* slave);
        void onSlaveSetReal                 (FMIClient* slave);
        void onSlaveGotReal                 (FMIClient* slave);
        void onSlaveGotState                (FMIClient* slave);
        void onSlaveSetState                (FMIClient* slave);
        void onSlaveFreedState              (FMIClient* slave);
        void onSlaveDirectionalDerivative   (FMIClient* slave);

        /// Set communication timestep
        void setTimeStep(double timeStep);

        /// Enable or disable end time
        void setEnableEndTime(bool enable);

        /// Set the simulation end time
        void setEndTime(double endTime);

        /// Set method for weak coupling
        void setWeakMethod(WeakCouplingAlgorithm algorithm);

        /// Create strong connection
        void addStrongConnection(sc::Constraint* conn);

        /// Create weak connection between the slaves
        void createWeakConnection(FMIClient* slaveA, FMIClient* slaveB, int valueReferenceA, int valueReferenceB);

        /// Start simulation
        void simulate();

        void getXmlForSlaves();
        /// Request all slaves to instantiate
        void instantiateSlaves();
        void initializeSlaves();
        void stepSlaves(bool forFutureVelocities);
        void getFutureVelocities();
        void fetchDirectionalDerivatives();
        void transferStrongConnectionData();
        void getWeakConnectionReals();
        void setWeakConnectionReals();
        void getSlaveStates();
        void setSlaveStates();
        void setStrongCouplingForces();
        void getStrongCouplingReals();

        /// Returns true if all TCP clients have the given state
        bool allClientsHaveState(FMIClientState state);

        /// "State machine" tick
        void tick();
    };
};

#endif /* MASTER_H_ */
