#include <string>
#include <fmitcp/EventPump.h>
#include <fmitcp/Logger.h>
#include "stdlib.h"
#include "string.h"

#include "master/Master.h"
#include "master/FMIClient.h"
#include "master/StrongConnection.h"
#include "master/WeakConnection.h"
#include "common/url_parser.h"

using namespace fmitcp_master;

Master::Master(const fmitcp::Logger& logger, fmitcp::EventPump* pump){
    m_logger = logger;
    m_pump = pump;
    init();
}

void Master::init(){

    // Set state
    setState(MASTER_STATE_CONNECTING_SLAVES);
    m_slaveIdCounter = 0;

    m_relativeTolerance = 0.0001;
    m_startTime = 0;
    m_endTimeDefined = false;
    m_endTime = 10;
    m_time = 0;
}

Master::~Master(){

    // Delete all connections
    for (int i = 0; i < m_weakConnections.size(); ++i){
        delete m_weakConnections[i];
    }

    // Delete all remaining slaves
    for (int i = 0; i < m_slaves.size(); ++i){
        delete m_slaves[i];
    }
}

fmitcp::EventPump * Master::getEventPump(){
    return m_pump;
}

fmitcp::Logger * Master::getLogger(){
    return &m_logger;
};


FMIClient* Master::connectSlave(std::string uri){
    struct parsed_url * url = parse_url(uri.c_str());
    long port = atoi(url->port);

    FMIClient* client = new FMIClient(this,m_pump);
    client->connect(url->host,port);
    m_slaves.push_back(client);

    int slaveId = m_slaveIdCounter++;
    client->setId(slaveId);

    m_logger.log(fmitcp::Logger::LOG_DEBUG,"Connected slave id=%d: %s\n",slaveId,uri.c_str());

    parsed_url_free(url);

    return client;
}

void Master::simulate(){

    /*
    // If strong coupling was added, we need to set up the system for that
    if(m_strongConnections.size() > 0){

        // Create sc-slaves for each slave that has strong connections
        for(int i=0; i<m_slaves.size(); i++){
            m_strongCouplingSlaves.push_back(new sc::Slave());
        }

        // Create an sc-connector for each strong coupling connector
        for(int i=0; i<m_strongConnections.size(); i++){

            sc::Slave* slave = m_strongCouplingSlaves[i];

            sc::Connector * conn = new sc::Connector();
            slave->addConnector(conn);

            // Store so we can delete later
            m_strongCouplingConnectors.push_back(conn);

            // So we can reach the connection later
            conn->m_userData = (void*)m_strongConnections[i];

            // Note: Must add slave to solver *after* adding connectors to slave
            m_strongCouplingSolver.addSlave(slave);
        }
    }
    */

    m_pump->startEventLoop();
    tick();
}

FMIClient * Master::getSlave(int id){
    for(int i=0; i<m_slaves.size(); i++){
        if(m_slaves[i]->getId() == id){
            return m_slaves[i];
        }
    }
    return NULL;
}

void Master::slaveConnected(FMIClient * client){
    m_logger.log(fmitcp::Logger::LOG_NETWORK,"Connected to slave %d.\n",client->getId());
    tick();
}

void Master::slaveError(FMIClient * client){
    m_logger.log(fmitcp::Logger::LOG_NETWORK,"Slave %d error!\n", client->getId());
    m_pump->exitEventLoop();
}

void Master::slaveDisconnected(FMIClient* client){
    m_logger.log(fmitcp::Logger::LOG_NETWORK,"Disconnected slave.\n");

    // Remove from slave vector
    for(int i=0; i<m_slaves.size(); i++){
        if(m_slaves[i] == client){
            delete m_slaves[i];
            m_slaves.erase(m_slaves.begin()+i);
            break;
        }
    }

    // No slaves left - exit
    // TODO: Move this to the tick function?
    if(m_slaves.size() == 0)
        m_pump->exitEventLoop();
}

void Master::getXmlForSlaves() {
  setState(MASTER_STATE_GETTING_XML);
  for(int i=0; i<m_slaves.size(); i++) {
    m_logger.log(fmitcp::Logger::LOG_DEBUG,"Getting XML slave %d...\n", i);
    m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_GET_XML;
    m_slaves[i]->getXml(0, 0);
  }
}

void Master::instantiateSlaves() {
  setState(MASTER_STATE_INSTANTIATING_SLAVES);
  for(int i=0; i<m_slaves.size(); i++){
    m_logger.log(fmitcp::Logger::LOG_DEBUG,"Instantiating slave %d...\n", i);
    m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_INSTANTIATE_SLAVE;
    m_slaves[i]->fmi2_import_instantiate(0);
  }
}

void Master::initializeSlaves() {
    setState(MASTER_STATE_INITIALIZING_SLAVES);
    for(int i=0; i<m_slaves.size(); i++){
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Initializing slave %d...\n", i);
        m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_INITIALIZE_SLAVE;
        m_slaves[i]->fmi2_import_initialize_slave(0, 0, true, m_relativeTolerance, m_startTime, m_endTimeDefined, m_endTime);
    }
}

void Master::getStrongCouplingReals(){
    setState(MASTER_STATE_GET_STRONG_CONNECTOR_STATES);

    for(int i=0; i<m_slaves.size(); i++){
        FMIClient * s = m_slaves[i];
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Getting strong coupling reals for slave %d...\n", i);
        s->m_state = FMICLIENT_STATE_WAITING_GET_REAL;

        // Get valuerefs of the values we need.
        std::vector<int> valueRefs = s->getStrongConnectorValueReferences();

        // Send request
        s->fmi2_import_get_real(0,0,valueRefs);
    }
}

void Master::fetchDirectionalDerivatives() {
    setState(MASTER_STATE_GETTING_DIRECTIONAL_DERIVATIVES);

    std::vector<sc::Equation*> eqs;
    m_strongCouplingSolver.getEquations(&eqs);

    std::vector<int> v_ref;
    std::vector<int> z_ref;
    std::vector<double> dv;

    for(int i=0; i<m_slaves.size(); i++)
        m_slaves[i]->m_numDirectionalDerivativesLeft = 0;

    // Get jacobian information
    for (int j = 0; j < eqs.size(); ++j){
        sc::Equation * eq = eqs[j];

        FMIClient * slaveA = (FMIClient *)eq->getConnA()->m_userData;
        FMIClient * slaveB = (FMIClient *)eq->getConnB()->m_userData;

        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Equation %d, slaves %d and %d\n",j,slaveA->getId(),slaveB->getId());

        slaveA->m_state = slaveB->m_state = FMICLIENT_STATE_WAITING_DIRECTIONALDERIVATIVES;

        sc::Vec3 spatSeed, rotSeed;

        // Set jacobians
        eq->getSpatialJacobianSeedA(spatSeed);
        eq->getRotationalJacobianSeedA(rotSeed);
        //slaveA->fmi2_import_get_directional_derivative(0, 0, v_ref, z_ref, dv);
        //slaveA->m_numDirectionalDerivativesLeft++;
        slaveA->pushDirectionalDerivativeRequest(0, v_ref, z_ref, dv);
        //m_pump->tick();
        //slaveA->getDirectionalDerivative(ddSpatial,ddRotational,slaveA->m_position,spatSeed,rotSeed, dt);
        //printf("Eq %d:\n", j);
        //printf("A = (%f %f %f)\n", ddSpatial[0], ddSpatial[1], ddSpatial[2]);
        //eq->setSpatialJacobianA(ddSpatial);
        //eq->setRotationalJacobianA(ddRotational);

        ////printf("A = (%f %f %f)\n", ddRotational[0], ddRotational[1], ddRotational[2]);

        eq->getSpatialJacobianSeedB(spatSeed);
        eq->getRotationalJacobianSeedB(rotSeed);
        //slaveB->fmi2_import_get_directional_derivative(0, 0, v_ref, z_ref, dv);
        //slaveB->m_numDirectionalDerivativesLeft++;
        slaveB->pushDirectionalDerivativeRequest(0, v_ref, z_ref, dv);
        //m_pump->tick();
        //slaveB->getDirectionalDerivative(ddSpatial,ddRotational,slaveB->m_position,spatSeed,rotSeed, dt);
        //printf("B: dd=(%f %f %f), seed=(%f %f %f)\n", ddSpatial[0], ddSpatial[1], ddSpatial[2], spatSeed[0], spatSeed[1], spatSeed[2]);
        //eq->setSpatialJacobianB(ddSpatial);
        //eq->setRotationalJacobianB(ddRotational);

        //printf("t=%f wA=(%g %g %g)\n",t,bodyA->m_torque[0],bodyA->m_torque[1],bodyA->m_torque[2]);
        //printf("t=%f wB=(%g %g %g)\n",t,bodyB->m_torque[0],bodyB->m_torque[1],bodyB->m_torque[2]);
    }

    // Start sending requests
    for(int i=0; i<m_slaves.size(); i++){
        FMIClient* slave = m_slaves[i];
        if(slave->numDirectionalDerivativeRequests() == 0){
            slave->m_state = FMICLIENT_STATE_DONE_DIRECTIONALDERIVATIVES;
        } else {
            slave->shiftExecuteDirectionalDerivativeRequest();
        }
    }

    /*
    for(int i=0; i<m_slaves.size(); i++){

        std::vector<int> v_ref;
        std::vector<int> z_ref;
        std::vector<double> dv;

        // Loop over all strong connectors
        for(int j=0; j<m_slaves[i]->getNumConnectors(); j++){

            // v_ref <=> force
            std::vector<int> forceRefs = m_slaves[i]->getConnector(i)->getForceValueRefs();

            // z_ref <=> velocity
            std::vector<int> veloRefs = m_slaves[i]->getConnector(i)->getVelocityValueRefs();

            // dv is the Jacobian entry.
            // TODO: need to get the corresponding Equation from the strong connection library

            // Store
            for(int k=0; k<v_ref.size(); k++){
                v_ref.push_back(forceRefs[k]);
                z_ref.push_back(veloRefs[k]);
                dv.push_back(0); // for now
            }
        }

        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Getting directional derivatives from slave %d...\n", i);
        m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_DIRECTIONALDERIVATIVES;
        // TODO fill these with seeds
        m_slaves[i]->fmi2_import_get_directional_derivative(0, 0, v_ref, z_ref, dv);
    }
    */
}

void Master::getSlaveStates() {
    setState(MASTER_STATE_GETTING_STATES);
    for(int i=0; i<m_slaves.size(); i++){
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Getting state from slave %d...\n", i);
        m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_GET_STATE;
        m_slaves[i]->fmi2_import_get_fmu_state(0,0);
    }
}

void Master::setSlaveStates() {
    setState(MASTER_STATE_SETTING_STATES);
    for(int i=0; i<m_slaves.size(); i++){
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Setting state for slave %d...\n", i);
        m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_SET_STATE;
        m_slaves[i]->fmi2_import_set_fmu_state(0,0,0);
    }
}

void Master::stepSlaves(bool forFutureVelocities){
    if(forFutureVelocities)
        setState(MASTER_STATE_STEPPING_SLAVES_FOR_FUTURE_VELO);
    else
        setState(MASTER_STATE_STEPPING_SLAVES);
    for(int i=0; i<m_slaves.size(); i++){
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Stepping slave %d...\n", i);
        //m_slaves[i]->fmi2_import_do_step(0, 0, m_relativeTolerance, m_startTime, m_endTimeDefined, m_endTime);
        m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_DOSTEP;
        m_slaves[i]->fmi2_import_do_step(0,0,m_time,m_timeStep,true);
    }
    if(!forFutureVelocities)
        m_time += m_timeStep;
}

void Master::getFutureVelocities(){
    setState(MASTER_STATE_GETTING_FUTURE_VELO);
    for(int i=0; i<m_slaves.size(); i++){
        FMIClient* s = m_slaves[i];
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Getting future velocity from slave %d...\n", i);
        s->m_state = FMICLIENT_STATE_WAITING_GET_REAL;
        s->fmi2_import_get_real(0,0,s->getStrongSeedOutputValueReferences());
    }
}

void Master::setStrongCouplingForces(){
    setState(MASTER_STATE_SETTING_STRONG_COUPLING_FORCES);

    for(int i=0; i<m_slaves.size(); i++){
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Setting strong coupling forces for slave %d...\n", i);
        m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_SET_REAL;
        std::vector<int> valueRefs;
        std::vector<double> values;
        m_slaves[i]->fmi2_import_set_real(0,0,valueRefs,values);
    }
}

void Master::getWeakConnectionReals(){
    setState(MASTER_STATE_GET_WEAK_REALS);

    for(int i=0; i<m_slaves.size(); i++){
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Getting weak coupling reals for slave %d...\n", i);
        m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_GET_REAL;
        std::vector<int> valueRefs;
        m_slaves[i]->fmi2_import_get_real(0,0,valueRefs);
    }
}

void Master::setWeakConnectionReals(){
    setState(MASTER_STATE_SET_WEAK_REALS);

    for(int i=0; i<m_slaves.size(); i++){
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"Setting weak coupling reals for slave %d...\n", i);
        m_slaves[i]->m_state = FMICLIENT_STATE_WAITING_SET_REAL;
        std::vector<int> valueRefs;
        std::vector<double> values;
        m_slaves[i]->fmi2_import_set_real(0,0,valueRefs,values);
    }
}

void Master::setState(MasterState state){
    m_state = state;

    switch(m_state){
    case MASTER_STATE_START:                            m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_START ===\n");                             break;
    case MASTER_STATE_CONNECTING_SLAVES:                m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_CONNECTING_SLAVES ===\n");                 break;
    case MASTER_STATE_START_SIMLOOP:                    m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_START_SIMLOOP ===\n");                     break;
    case MASTER_STATE_GETTING_VERSION:                  m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_GETTING_VERSION ===\n");                   break;
    case MASTER_STATE_GETTING_XML:                      m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_GETTING_XML ===\n");                       break;
    case MASTER_STATE_INSTANTIATING_SLAVES:             m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_INSTANTIATING_SLAVES ===\n");              break;
    case MASTER_STATE_INITIALIZING_SLAVES:              m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_INITIALIZING_SLAVES ===\n");               break;
    case MASTER_STATE_TRANSFERRING_WEAK:                m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_TRANSFERRING_WEAK ===\n");                 break;
    case MASTER_STATE_GETTING_STATES:                   m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_GETTING_STATES ===\n");                    break;
    case MASTER_STATE_STEPPING_SLAVES_FOR_FUTURE_VELO:  m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_STEPPING_SLAVES_FOR_FUTURE_VELO ===\n");   break;
    case MASTER_STATE_SETTING_STATES:                   m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_SETTING_STATES ===\n");                    break;
    case MASTER_STATE_GETTING_DIRECTIONAL_DERIVATIVES:  m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_GETTING_DIRECTIONAL_DERIVATIVES ===\n");   break;
    case MASTER_STATE_SETTING_STRONG_COUPLING_FORCES:   m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_SETTING_STRONG_COUPLING_FORCES ===\n");    break;
    case MASTER_STATE_STEPPING_SLAVES:                  m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_STEPPING_SLAVES ===\n");                   break;
    case MASTER_STATE_GET_WEAK_REALS:                   m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_GET_WEAK_REALS ===\n");                    break;
    case MASTER_STATE_SET_WEAK_REALS:                   m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_SET_WEAK_REALS ===\n");                    break;
    case MASTER_STATE_GET_STRONG_CONNECTOR_STATES:      m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_GET_STRONG_CONNECTOR_STATES ===\n");       break;
    case MASTER_STATE_DONE:                             m_logger.log(fmitcp::Logger::LOG_DEBUG,"=== MASTER_STATE_DONE ===\n");                              break;

    default:
        m_logger.log(fmitcp::Logger::LOG_DEBUG,"State not recognized: %d\n",m_state);
        break;
    }
}

bool Master::allClientsHaveState(FMIClientState state){
    for(int i=0; i<m_slaves.size(); i++){
        if(m_slaves[i]->m_state != state)
            return false;
    }
    return true;
}

// Simulation loop logic. TODO: use finite state machine library for this?
void Master::tick(){

    bool allConnected, allInstantiated, allInitialized, allReady;

    switch(m_state){

    case MASTER_STATE_CONNECTING_SLAVES:

        // Check if all slaves are connected.
        allConnected = true;
        for(int i=0; i<m_slaves.size(); i++){
            if(!m_slaves[i]->isConnected()){
                allConnected = false;
                break;
            }
        }

        if(!allConnected)
            break;

        // Enough slaves connected. Start getting XML!
        getXmlForSlaves();
        break;

    case MASTER_STATE_GETTING_VERSION:
        break;

    case MASTER_STATE_GETTING_XML:
      // Check if all are ready
      if(!allClientsHaveState(FMICLIENT_STATE_DONE_GET_XML))
        break;

      // All slaves have got the xml. Start instantiating!
      instantiateSlaves();
      break;

    case MASTER_STATE_INSTANTIATING_SLAVES:
      // Check if all are ready
      if(!allClientsHaveState(FMICLIENT_STATE_DONE_INSTANTIATE_SLAVE))
        break;

      // All slaves are instantiated.
      // Should set initial values here. TODO!
      initializeSlaves();
      break;

    case MASTER_STATE_INITIALIZING_SLAVES:
        // Check if all are ready
        if(!allClientsHaveState(FMICLIENT_STATE_DONE_INITIALIZE_SLAVE))
            break;
        // If all are ready, just continue to the case below, start simloop

    case MASTER_STATE_START_SIMLOOP:

        // All slaves are initialized.
        if(m_strongConnections.size()){
            // There are strong connections. We must now get states.
            getSlaveStates();

        } else if(m_weakConnections.size()){
            getWeakConnectionReals();

        } else {
            // No connections at all, we can now do final step
            stepSlaves(false);
        }
        break;


    case MASTER_STATE_GETTING_STATES:
        if(allClientsHaveState(FMICLIENT_STATE_DONE_GET_STATE))
            stepSlaves(true); // Step to get future velocities
        break;

    case MASTER_STATE_STEPPING_SLAVES_FOR_FUTURE_VELO:
        if(allClientsHaveState(FMICLIENT_STATE_DONE_DOSTEP)){

            // TODO store future velocities in the connectors!!
            getFutureVelocities();
        }
        break;

    case MASTER_STATE_GETTING_FUTURE_VELO:
        // All ready?
        if(!allClientsHaveState(FMICLIENT_STATE_DONE_GET_REAL))
            break;

        // TODO: Store future velocities

        setSlaveStates(); // Rewind!

        break;

    case MASTER_STATE_SETTING_STATES:
        if(allClientsHaveState(FMICLIENT_STATE_DONE_SET_STATE)){
            // Done rewinding.
            getStrongCouplingReals();
        }
        break;

    case MASTER_STATE_GET_STRONG_CONNECTOR_STATES:

        // All ready?
        if(!allClientsHaveState(FMICLIENT_STATE_DONE_GET_REAL))
            break;

        // The following must be called whenever connector values are changed
        m_strongCouplingSolver.updateConstraints();

        // Get directional derivatives
        fetchDirectionalDerivatives();

        break;

    case MASTER_STATE_GETTING_DIRECTIONAL_DERIVATIVES:
        // All ready?
        if(!allClientsHaveState(FMICLIENT_STATE_DONE_DIRECTIONALDERIVATIVES))
            break;

        // Got directional derivatives and future velocities.
        // TODO: Run strong coupling library

        // Apply strong coupling forces to slaves
        setStrongCouplingForces();

        break;

    case MASTER_STATE_SETTING_STRONG_COUPLING_FORCES:
        // Check if all strong coupling forces are applied
        if(!allClientsHaveState(FMICLIENT_STATE_DONE_SET_REAL))
            break;

        // All strong coupling forces are set. Now transfer weak.
        if(m_weakConnections.size() > 0)
            getWeakConnectionReals();
        else
            stepSlaves(false); // Jump to final step

        break;

    case MASTER_STATE_GET_WEAK_REALS:
        if(!allClientsHaveState(FMICLIENT_STATE_DONE_GET_REAL))
            break;

        setWeakConnectionReals();
        break;

    case MASTER_STATE_SET_WEAK_REALS:
        if(!allClientsHaveState(FMICLIENT_STATE_DONE_SET_REAL))
            break;

        // Done with eveything. step!
        stepSlaves(false);

        break;

    case MASTER_STATE_STEPPING_SLAVES:
        if(allClientsHaveState(FMICLIENT_STATE_DONE_DOSTEP)){
            // Next step?
            if((m_endTimeDefined && m_time < m_endTime) || !m_endTimeDefined){
                //stepSlaves(false);
                setState(MASTER_STATE_START_SIMLOOP);
                tick();
            } else {
                // We are done with the simulation!
                setState(MASTER_STATE_DONE);
                m_pump->exitEventLoop();
            }
        }
        break;

    case MASTER_STATE_DONE:
        break;
    }
}

void Master::addStrongConnection(StrongConnection* conn){
    m_strongCouplingSolver.addConstraint(conn->getConstraint());
    m_strongConnections.push_back(conn);
};

void Master::createWeakConnection(FMIClient* slaveA, FMIClient* slaveB, int valueReferenceA, int valueReferenceB){
    m_weakConnections.push_back(new WeakConnection(slaveA,slaveB,valueReferenceA,valueReferenceB));
}

void Master::setTimeStep(double timeStep){
    m_timeStep = timeStep;
}

void Master::setEnableEndTime(bool enable){
    m_endTimeDefined = enable;
}

void Master::setEndTime(double endTime){
    m_endTime = endTime;
}

void Master::setWeakMethod(WeakCouplingAlgorithm algorithm){
    m_method = algorithm;
}

void Master::onSlaveGetXML(FMIClient * slave) {
  slave->m_state = FMICLIENT_STATE_DONE_GET_XML;
  tick();
};

void Master::onSlaveInstantiated(FMIClient* slave){
    slave->m_state = FMICLIENT_STATE_DONE_INSTANTIATE_SLAVE;
    slave->m_isInstantiated = true;
    tick();
};

void Master::onSlaveInitialized(FMIClient* slave){
    slave->m_state = FMICLIENT_STATE_DONE_INITIALIZE_SLAVE;
    tick();
};

void Master::onSlaveTerminated(FMIClient* slave){
    slave->m_state = FMICLIENT_STATE_DONE_TERMINATE_SLAVE;
    tick();
};

void Master::onSlaveFreed(FMIClient* slave){
    tick();
};

void Master::onSlaveStepped(FMIClient* slave){
    slave->m_state = FMICLIENT_STATE_DONE_DOSTEP;
    tick();
};

void Master::onSlaveGotVersion(FMIClient* slave){
    tick();
};

void Master::onSlaveSetReal(FMIClient* slave){
    slave->m_state = FMICLIENT_STATE_DONE_SET_REAL;
    tick();
};

void Master::onSlaveGotReal(FMIClient* slave){
    slave->m_state = FMICLIENT_STATE_DONE_GET_REAL;

    if(m_state == MASTER_STATE_GET_STRONG_CONNECTOR_STATES){

        // Got real values for connectors. Set em.
        slave->setConnectorValues(slave->getStrongConnectorValueReferences(), slave->m_getRealValues);

    } else if(m_state == MASTER_STATE_GETTING_FUTURE_VELO){

        // Got future velocities. Set them in the connector
        slave->setConnectorFutureVelocities(slave->getStrongSeedOutputValueReferences(), slave->m_getRealValues);

    }

    tick();
};

void Master::onSlaveGotState(FMIClient* slave){
    slave->m_state = FMICLIENT_STATE_DONE_GET_STATE;
    tick();
};

void Master::onSlaveSetState(FMIClient* slave){
    slave->m_state = FMICLIENT_STATE_DONE_SET_STATE;
    tick();
};

void Master::onSlaveFreedState(FMIClient* slave){
    slave->m_state = FMICLIENT_STATE_DONE_FREE_STATE;
    tick();
};

void Master::onSlaveDirectionalDerivative(FMIClient* slave){
    slave->m_numDirectionalDerivativesLeft--;

    m_logger.log(fmitcp::Logger::LOG_DEBUG,"Got directional derivative data from slave %d. Still %d to go...\n",slave->getId(),slave->numDirectionalDerivativeRequests());

    if(slave->numDirectionalDerivativeRequests() == 0){
        slave->m_state = FMICLIENT_STATE_DONE_DIRECTIONALDERIVATIVES;
    } else {
        slave->shiftExecuteDirectionalDerivativeRequest();
    }

    tick();
};
