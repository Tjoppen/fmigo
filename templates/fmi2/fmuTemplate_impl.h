/* ---------------------------------------------------------------------------*
 * fmuTemplate.c
 * Implementation of the FMI interface based on functions and macros to
 * be defined by the includer of this file.
 * If FMI_COSIMULATION is defined, this implements "FMI for Co-Simulation 2.0",
 * otherwise "FMI for Model Exchange 2.0".
 * The "FMI for Co-Simulation 2.0", implementation assumes that exactly the
 * following capability flags are set to fmi2True:
 *    canHandleVariableCommunicationStepSize, i.e. fmiDoStep step size can vary
 * and all other capability flags are set to default, i.e. to fmi2False or 0.
 *
 * Revision history
 *  07.03.2014 initial version released in FMU SDK 2.0.0
 *  02.04.2014 allow modules to request termination of simulation, better time
 *             event handling, initialize() moved from fmiEnterInitialization to
 *             fmiExitInitialization, correct logging message format in fmiDoStep.
 *
 * Author: Adrian Tirea
 * Copyright QTronic GmbH. All rights reserved.
 * ---------------------------------------------------------------------------*/

// macro to be used to log messages. The macro check if current
// log category is valid and, if true, call the logger provided by simulator.
#define FILTERED_LOG(instance, status, categoryIndex, message, ...) if (isCategoryLogged(instance, categoryIndex)) \
        instance->functions->logger(instance->functions->componentEnvironment, instance->instanceName, status, \
        logCategoriesNames[categoryIndex], message, ##__VA_ARGS__);

static fmi2String logCategoriesNames[] = {"logAll", "logError", "logFmiCall", "logEvent"};

// array of value references of states
#if NUMBER_OF_STATES>0
fmi2ValueReference vrStates[NUMBER_OF_STATES] = STATES;
#endif

// ---------------------------------------------------------------------------
// Private helpers logger
// ---------------------------------------------------------------------------

// return fmi2True if logging category is on. Else return fmi2False.
static fmi2Boolean isCategoryLogged(ModelInstance *comp, int categoryIndex) {
    if (categoryIndex < NUMBER_OF_CATEGORIES
        && (comp->logCategories[categoryIndex] || comp->logCategories[LOG_ALL])) {
        return fmi2True;
    }
    return fmi2False;
}

// ---------------------------------------------------------------------------
// Private helpers used below to validate function arguments
// ---------------------------------------------------------------------------

static fmi2Boolean invalidNumber(ModelInstance *comp, const char *f, const char *arg, int n, int nExpected) {
    if (n != nExpected) {
        comp->s.state = modelError;
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Invalid argument %s = %d. Expected %d.", f, arg, n, nExpected)
        return fmi2True;
    }
    return fmi2False;
}

static fmi2Boolean invalidState(ModelInstance *comp, const char *f, int statesExpected) {
    if (!comp)
        return fmi2True;
    if (!(comp->s.state & statesExpected)) {
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Illegal call sequence. got %i, expected %i", f, comp->s.state, statesExpected)
        comp->s.state = modelError;
        return fmi2True;
    }
    return fmi2False;
}

static fmi2Boolean nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p) {
    if (!p) {
        comp->s.state = modelError;
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Invalid argument %s = NULL.", f, arg)
        return fmi2True;
    }
    return fmi2False;
}

#ifndef HAVE_GENERATED_GETTERS_SETTERS
static fmi2Boolean vrOutOfRange(ModelInstance *comp, const char *f, fmi2ValueReference vr, int end) {
    if (vr >= end) {
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Illegal value reference %u.", f, vr)
        comp->s.state = modelError;
        return fmi2True;
    }
    return fmi2False;
}
#endif

static fmi2Status unsupportedFunction(fmi2Component c, const char *fName, int statesExpected) {
    ModelInstance *comp = (ModelInstance *)c;
    fmi2CallbackLogger log = comp->functions->logger;
    if (invalidState(comp, fName, statesExpected))
        return fmi2Error;
    if (comp->loggingOn) log(c, comp->instanceName, fmi2OK, "log", fName);
    FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Function not implemented.", fName)
    return fmi2Error;
}

fmi2Status fmi2SetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]);
fmi2Status setString(fmi2Component comp, fmi2ValueReference vr, fmi2String value){
    return fmi2SetString(comp, &vr, 1, &value);
}

static void zeroState(state_t *s) {
    //zero s, except simulation pointer
#ifdef SIMULATION_TYPE
    memset(s, 0, sizeof(*s) - sizeof(s->simulation));
#else
    memset(s, 0, sizeof(*s));
#endif
}

// ---------------------------------------------------------------------------
// FMI functions: class methods not depending of a specific model instance
// ---------------------------------------------------------------------------

const char* fmi2GetTypesPlatform() {
    return fmi2TypesPlatform;
}

const char* fmi2GetVersion() {
    return fmi2Version;
}

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {
    // ignore arguments: nCategories, categories
    int i, j;
    ModelInstance *comp = (ModelInstance *)c;
    comp->loggingOn = loggingOn;

    for (j = 0; j < NUMBER_OF_CATEGORIES; j++) {
        comp->logCategories[j] = fmi2False;
    }
    for (i = 0; i < nCategories; i++) {
        fmi2Boolean categoryFound = fmi2False;
        for (j = 0; j < NUMBER_OF_CATEGORIES; j++) {
            if (strcmp(logCategoriesNames[j], categories[i]) == 0) {
                comp->logCategories[j] = loggingOn;
                categoryFound = fmi2True;
                break;
            }
        }
        if (!categoryFound) {
            comp->functions->logger(comp->componentEnvironment, comp->instanceName, fmi2Warning,
                logCategoriesNames[LOG_ERROR],
                "logging category '%s' is not supported by model", categories[i]);
        }
    }

    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetDebugLogging")
    return fmi2OK;
}

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID,
                            fmi2String fmuResourceLocation, const fmi2CallbackFunctions *functions,
                            fmi2Boolean visible, fmi2Boolean loggingOn) {
    // ignoring arguments: fmuResourceLocation, visible
    ModelInstance *comp;
    if (!functions->logger) {
        return NULL;
    }

    if (!functions->allocateMemory || !functions->freeMemory) {
        functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
                "fmi2Instantiate: Missing callback function.");
        return NULL;
    }
    if (!instanceName || strlen(instanceName) == 0) {
        functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
                "fmi2Instantiate: Missing instance name.");
        return NULL;
    }
    if (strcmp(fmuGUID, MODEL_GUID)) {
        functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
                "fmi2Instantiate: Wrong GUID %s. Expected %s.", fmuGUID, MODEL_GUID);
        return NULL;
    }
    comp = (ModelInstance *)functions->allocateMemory(1, sizeof(ModelInstance));
    if (comp) {
        int i;
        comp->instanceName = functions->allocateMemory(1 + strlen(instanceName), sizeof(char));

        // UMIT: log errors only, if logging is on. We don't have to enable all of them,
        // to quote the spec: "Which LogCategories the FMU sets is unspecified."
        // fmi2SetDebugLogging should be called to choose specific categories.
        for (i = 0; i < NUMBER_OF_CATEGORIES; i++) {
            comp->logCategories[i] = 0;
        }
        comp->logCategories[LOG_ERROR] = loggingOn;
    }
    if (!comp || !comp->instanceName) {

        functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
            "fmi2Instantiate: Out of memory.");
        return NULL;
    }
    strcpy(comp->instanceName, instanceName);
    comp->type = fmuType;
    comp->functions = functions;
    comp->componentEnvironment = functions->componentEnvironment;
    comp->loggingOn = loggingOn;
    zeroState(&comp->s);
    comp->s.state = modelInstantiated;
#ifdef HAVE_DEFAULTS
    comp->s.md = defaults;
#endif

    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Instantiate: GUID=%s", fmuGUID)

    return comp;
}

void fmi2FreeInstance(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (!comp) return;
    if (invalidState(comp, "fmi2FreeInstance", modelTerminated))
        return;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2FreeInstance")
    if (comp->instanceName) comp->functions->freeMemory(comp->instanceName);
#ifdef SIMULATION_FREE
    SIMULATION_FREE(comp->s.simulation);
#endif
    comp->functions->freeMemory(comp);
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance,
                            fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {

    // ignore arguments: stopTimeDefined, stopTime
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetupExperiment", modelInstantiated))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetupExperiment: toleranceDefined=%d tolerance=%g",
        toleranceDefined, tolerance)

    comp->s.time = startTime;
    return fmi2OK;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2EnterInitializationMode", modelInstantiated))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2EnterInitializationMode")

    comp->s.state = modelInitializationMode;
    return fmi2OK;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2ExitInitializationMode", modelInitializationMode))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2ExitInitializationMode")

#ifdef SIMULATION_INIT
    SIMULATION_INIT(&comp->s);
#endif
    comp->s.state = modelInitialized;
    return fmi2OK;
}

fmi2Status fmi2Terminate(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2Terminate", modelInitialized|modelStepping))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Terminate")

    comp->s.state = modelTerminated;
    return fmi2OK;
}

fmi2Status fmi2Reset(fmi2Component c) {
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2Reset", modelInitialized|modelStepping))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Reset")

    zeroState(&comp->s);
    comp->s.state = modelInstantiated;
    return fmi2OK;
}

fmi2Status fmi2GetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
#ifndef HAVE_GENERATED_GETTERS_SETTERS
    int i;
#endif
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetReal", modelInitializationMode|modelInitialized|modelStepping|modelError))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetReal", "vr[]", vr))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetReal", "value[]", value))
        return fmi2Error;
#ifdef HAVE_GENERATED_GETTERS_SETTERS
    return generated_fmi2GetReal(&comp->s.md, vr, nvr, value);
#else
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2GetReal", vr[i], NUMBER_OF_REALS))
            return fmi2Error;
        value[i] = comp->s.r[vr[i]];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetReal: #r%u# = %.16g", vr[i], value[i])
    }
    return fmi2OK;
#endif
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
#ifndef HAVE_GENERATED_GETTERS_SETTERS
    int i;
#endif
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetInteger", modelInitializationMode|modelInitialized|modelStepping|modelError))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetInteger", "vr[]", vr))
            return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetInteger", "value[]", value))
            return fmi2Error;
#ifdef HAVE_GENERATED_GETTERS_SETTERS
    return generated_fmi2GetInteger(&comp->s.md, vr, nvr, value);
#else
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2GetInteger", vr[i], NUMBER_OF_INTEGERS))
            return fmi2Error;
        value[i] = comp->s.i[vr[i]];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetInteger: #i%u# = %d", vr[i], value[i])
    }
    return fmi2OK;
#endif
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
#ifndef HAVE_GENERATED_GETTERS_SETTERS
    int i;
#endif
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetBoolean", modelInitializationMode|modelInitialized|modelStepping|modelError))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetBoolean", "vr[]", vr))
            return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetBoolean", "value[]", value))
            return fmi2Error;
#ifdef HAVE_GENERATED_GETTERS_SETTERS
    return generated_fmi2GetBoolean(&comp->s.md, vr, nvr, value);
#else
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2GetBoolean", vr[i], NUMBER_OF_BOOLEANS))
            return fmi2Error;
        value[i] = comp->s.b[vr[i]];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetBoolean: #b%u# = %s", vr[i], value[i]? "true" : "false")
    }
    return fmi2OK;
#endif
}

fmi2Status fmi2GetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    return unsupportedFunction(c, "fmi2GetString",
        modelInitializationMode|modelInitialized|modelStepping|modelError);
}

fmi2Status fmi2SetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
#ifndef HAVE_GENERATED_GETTERS_SETTERS
    int i;
#endif
    fmi2Status ret;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetReal", modelInstantiated|modelInitializationMode|modelInitialized|modelStepping))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetReal", "vr[]", vr))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetReal", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetReal: nvr = %d", nvr)
#ifdef HAVE_GENERATED_GETTERS_SETTERS
    ret = generated_fmi2SetReal(&comp->s.md, vr, nvr, value);
#else
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2SetReal", vr[i], NUMBER_OF_REALS))
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetReal: #r%d# = %.16g", vr[i], value[i])
        comp->s.r[vr[i]] = value[i];
    }
    ret = fmi2OK;
#endif
#ifdef HAVE_INITIALIZATION_MODE
    //we could do something similar in other modes too
    //we just have to be careful what we use for states going into sync_out()
    if (comp->s.state == modelInitializationMode) {
        int N = get_initial_states_size(&comp->s);
        double *initials = calloc(N, sizeof(double));
        get_initial_states(&comp->s, initials);
        sync_out(N, initials, &comp->s);
        free(initials);
    }
#endif
    return ret;
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
#ifndef HAVE_GENERATED_GETTERS_SETTERS
    int i;
#endif
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetInteger", modelInstantiated|modelInitializationMode|modelInitialized|modelStepping))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetInteger", "vr[]", vr))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetInteger", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetInteger: nvr = %d", nvr)
#ifdef HAVE_GENERATED_GETTERS_SETTERS
    return generated_fmi2SetInteger(&comp->s.md, vr, nvr, value);
#else
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2SetInteger", vr[i], NUMBER_OF_INTEGERS))
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetInteger: #i%d# = %d", vr[i], value[i])
        comp->s.i[vr[i]] = value[i];
    }
    return fmi2OK;
#endif
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
#ifndef HAVE_GENERATED_GETTERS_SETTERS
    int i;
#endif
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetBoolean", modelInstantiated|modelInitializationMode|modelInitialized|modelStepping))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2SetBoolean", "vr[]", vr))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2SetBoolean", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetBoolean: nvr = %d", nvr)
#ifdef HAVE_GENERATED_GETTERS_SETTERS
    return generated_fmi2SetBoolean(&comp->s.md, vr, nvr, value);
#else
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2SetBoolean", vr[i], NUMBER_OF_BOOLEANS))
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetBoolean: #b%d# = %s", vr[i], value[i] ? "true" : "false")
        comp->s.b[vr[i]] = value[i];
    }
    return fmi2OK;
#endif
}

fmi2Status fmi2SetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    return unsupportedFunction(c, "fmi2SetString",
        modelInstantiated|modelInitializationMode|modelInitialized|modelStepping);
}

fmi2Status fmi2GetFMUstate (fmi2Component c, fmi2FMUstate* FMUstate) {
#if CAN_GET_SET_FMU_STATE
    ModelInstance *comp = (ModelInstance *)c;
    *FMUstate = comp->functions->allocateMemory(1, sizeof(comp->s));

#ifdef SIMULATION_TYPE
#ifndef SIMULATION_GET
//assume GSL
#define SIMULATION_GET cgsl_simulation_get
#endif
    SIMULATION_GET( &(( ModelInstance *) c)->s.simulation );
#endif
    memcpy(*FMUstate, &comp->s, sizeof(comp->s));
    return fmi2OK;
#else
    return fmi2Error;
#endif
}

fmi2Status fmi2SetFMUstate (fmi2Component c, fmi2FMUstate FMUstate) {
#if CAN_GET_SET_FMU_STATE
    ModelInstance *comp = (ModelInstance *)c;
    memcpy(&comp->s, FMUstate, sizeof(comp->s));

#ifdef SIMULATION_TYPE
#ifndef SIMULATION_SET
//assume GSL
#define SIMULATION_SET  cgsl_simulation_set
#endif
    SIMULATION_SET( &(( ModelInstance *) c)->s.simulation );
#endif
    return fmi2OK;
#else
    return fmi2Error;
#endif
}

fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
    ModelInstance *comp = (ModelInstance *)c;
    comp->functions->freeMemory(*FMUstate);
    *FMUstate = NULL;
    return fmi2OK;
}

fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t *size) {
    return fmi2Error; //disabled for now
    //ModelInstance *comp = (ModelInstance *)c;
    //*size = sizeof(comp->s);
    //return fmi2OK;
}

fmi2Status fmi2SerializeFMUstate (fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size) {
    return fmi2Error; //disabled for now
    //memcpy(serializedState, FMUstate, size);
    //return fmi2OK;
}

fmi2Status fmi2DeSerializeFMUstate (fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
    return fmi2Error; //disabled for now
    //memcpy(*FMUstate, serializedState, size);
    //return fmi2OK;
}

fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
                const fmi2ValueReference vKnown_ref[] , size_t nKnown, const fmi2Real dvKnown[], fmi2Real dvUnknown[]) {
#ifndef HAVE_DIRECTIONAL_DERIVATIVE
#error HAVE_DIRECTIONAL_DERIVATIVE not defined
#endif
#if HAVE_DIRECTIONAL_DERIVATIVE
    ModelInstance *comp = (ModelInstance *)c;
    size_t x, y;

    /**
     * Return partial derivatives of outputs wrt inputs, weighted by dvKnown
     */
    for (x = 0; x < nUnknown; x++) {
        dvUnknown[x] = 0;

        for (y = 0; y < nKnown; y++) {
            fmi2Real partial;
            fmi2Status status = getPartial(&comp->s, vUnknown_ref[x], vKnown_ref[y], &partial);

            if (status != fmi2OK) {
                fprintf(stderr, "Tried to get partial derivative of VR %i w.r.t VR %i, which doesn't exist or isn't defined\n", vUnknown_ref[x], vKnown_ref[y]);
                return status;
            }

            dvUnknown[x] += dvKnown[y] * partial;
        }
    }

    return fmi2OK;
#else
    return fmi2Error;
#endif
}

// ---------------------------------------------------------------------------
// Functions for FMI for Co-Simulation
// ---------------------------------------------------------------------------
#ifdef FMI_COSIMULATION
/* Simulating the slave */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                     const fmi2Integer order[], const fmi2Real value[]) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetRealInputDerivatives",
                     modelInstantiated|modelInitializationMode|modelInitialized|modelStepping)) {
        return fmi2Error;
    }
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetRealInputDerivatives: nvr= %d", nvr)
    FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "fmi2SetRealInputDerivatives: ignoring function call."
        " This model cannot interpolate inputs: canInterpolateInputs=\"fmi2False\"")
    return fmi2Error;
}

fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                      const fmi2Integer order[], fmi2Real value[]) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetRealOutputDerivatives", modelInitialized|modelStepping|modelError))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetRealOutputDerivatives: nvr= %d", nvr)
    FILTERED_LOG(comp, fmi2Error, LOG_ERROR,"fmi2GetRealOutputDerivatives: ignoring function call."
        " This model cannot compute derivatives of outputs: MaxOutputDerivativeOrder=\"0\"")
    for (i = 0; i < nvr; i++) value[i] = 0;
    return fmi2Error;
}

fmi2Status fmi2DoStep(fmi2Component cc, fmi2Real currentCommunicationPoint,
                    fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    ModelInstance *comp = (ModelInstance *)cc;

    if (invalidState(comp, "fmi2DoStep", modelInitialized|modelStepping))
        return fmi2Error;

    // model is in stepping state
    comp->s.state = modelStepping;

    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2DoStep: "
        "currentCommunicationPoint = %g, "
        "communicationStepSize = %g, "
        "noSetFMUStatePriorToCurrentPoint = fmi%s",
        currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint ? "True" : "False")

    if (communicationStepSize <= 0) {
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
            "fmi2DoStep: communication step size must be > 0. Fount %g.", communicationStepSize)
        return fmi2Error;
    }

#ifdef NEW_DOSTEP
    doStep(&comp->s, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);
#else
    doStep(&comp->s, currentCommunicationPoint, communicationStepSize);
#endif
    return fmi2OK;
}

fmi2Status fmi2CancelStep(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2CancelStep", modelStepping))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2CancelStep")
    FILTERED_LOG(comp, fmi2Error, LOG_ERROR,"fmi2CancelStep: Can be called when fmiDoStep returned fmiPending."
        " This is not the case.");
    return fmi2Error;
}

/* Inquire slave status */
static fmi2Status getStatus(char* fname, fmi2Component c, const fmi2StatusKind s) {
    const char *statusKind[3] = {"fmi2DoStepStatus","fmi2PendingStatus","fmi2LastSuccessfulTime"};
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, fname, modelInitialized|modelStepping))
            return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "$s: fmi2StatusKind = %s", fname, statusKind[s])

    switch(s) {
        case fmi2DoStepStatus: FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
            "%s: Can be called with fmi2DoStepStatus when fmiDoStep returned fmiPending."
            " This is not the case.", fname)
            break;
        case fmi2PendingStatus: FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
            "%s: Can be called with fmi2PendingStatus when fmiDoStep returned fmiPending."
            " This is not the case.", fname)
            break;
        case fmi2LastSuccessfulTime: FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
            "%s: Can be called with fmi2LastSuccessfulTime when fmiDoStep returned fmiDiscard."
            " This is not the case.", fname)
            break;
        case fmi2Terminated: FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
            "%s: Can be called with fmi2Terminated when fmiDoStep returned fmiDiscard."
            " This is not the case.", fname)
            break;
    }
    return fmi2Error;
}

fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status *value) {
    return getStatus("fmi2GetStatus", c, s);
}

fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real *value) {
    if (s == fmi2LastSuccessfulTime) {
        ModelInstance *comp = (ModelInstance *)c;
        *value = comp->s.time;
        return fmi2OK;
    }
    return getStatus("fmi2GetRealStatus", c, s);
}

fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer *value) {
    return getStatus("fmi2GetIntegerStatus", c, s);
}

fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean *value) {
    if (s == fmi2Terminated) {
        ModelInstance *comp = (ModelInstance *)c;
        *value = comp->s.eventInfo.terminateSimulation;
        return fmi2OK;
    }
    return getStatus("fmi2GetBooleanStatus", c, s);
}

fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String *value) {
    return getStatus("fmi2GetStringStatus", c, s);
}

// ---------------------------------------------------------------------------
// Functions for FMI for Model Exchange
// ---------------------------------------------------------------------------
#else
/* Enter and exit the different modes */
fmi2Status fmiEnterEventMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiEnterEventMode", modelStepping))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiEnterEventMode")

    return fmi2OK;
}

fmi2Status fmiNewDiscreteStates(fmi2Component c, fmi2EventInfo *eventInfo) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiNewDiscreteStates", modelInitialized|modelStepping))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiNewDiscreteStates")

    if (comp->s.state == modelStepping) {
        comp->s.eventInfo.newDiscreteStatesNeeded = fmi2False;
        comp->s.eventInfo.terminateSimulation = fmi2False;
        comp->s.eventInfo.nominalsOfContinuousStatesChanged = fmi2False;
        comp->s.eventInfo.valuesOfContinuousStatesChanged = fmi2False;
        comp->s.eventInfo.nextEventTimeDefined = fmi2False;
        comp->s.eventInfo.nextEventTime = 0; // next time event if nextEventTimeDefined = fmi2True

        eventUpdate(comp, &comp->s.eventInfo);
    }

    // model in stepping state
    comp->s.state = modelStepping;

    // copy internal eventInfo of component to output eventInfo
    eventInfo->newDiscreteStatesNeeded = comp->s.eventInfo.newDiscreteStatesNeeded;
    eventInfo->terminateSimulation = comp->s.eventInfo.terminateSimulation;
    eventInfo->nominalsOfContinuousStatesChanged = comp->s.eventInfo.nominalsOfContinuousStatesChanged;
    eventInfo->valuesOfContinuousStatesChanged = comp->s.eventInfo.valuesOfContinuousStatesChanged;
    eventInfo->nextEventTimeDefined = comp->s.eventInfo.nextEventTimeDefined;
    eventInfo->nextEventTime = comp->s.eventInfo.nextEventTime;

    return fmi2OK;
}

fmi2Status fmiEnterContinuousTimeMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiEnterContinuousTimeMode", modelStepping))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL,"fmiEnterContinuousTimeMode")
    return fmi2OK;
}

fmi2Status fmiCompletedIntegratorStep(fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint,
                                     fmi2Boolean *enterEventMode, fmi2Boolean *terminateSimulation) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiCompletedIntegratorStep", modelStepping))
        return fmi2Error;
    if (nullPointer(comp, "fmiCompletedIntegratorStep", "enterEventMode", enterEventMode))
        return fmi2Error;
    if (nullPointer(comp, "fmiCompletedIntegratorStep", "terminateSimulation", terminateSimulation))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL,"fmiCompletedIntegratorStep")
    *enterEventMode = fmi2False;
    *terminateSimulation = fmi2False;
    return fmi2OK;
}

/* Providing independent variables and re-initialization of caching */
fmi2Status fmiSetTime(fmi2Component c, fmi2Real time) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiSetTime", modelInstantiated|modelInitialized|modelStepping))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiSetTime: time=%.16g", time)
    comp->s.time = time;
    return fmi2OK;
}

fmi2Status fmiSetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx){
    ModelInstance *comp = (ModelInstance *)c;
    int i;
    if (invalidState(comp, "fmiSetContinuousStates", modelStepping))
        return fmi2Error;
    if (invalidNumber(comp, "fmiSetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;
    if (nullPointer(comp, "fmiSetContinuousStates", "x[]", x))
        return fmi2Error;
#ifdef HAVE_MODELDESCRIPTION_STRUCT
    return generated_fmi2SetContinuousStates(&comp->s.md, vrStates, nx, x);
#else
    for (i = 0; i < nx; i++) {
        fmi2ValueReference vr = vrStates[i];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiSetContinuousStates: #r%d#=%.16g", vr, x[i])
        assert(vr >= 0 && vr < NUMBER_OF_REALS);
        comp->s.r[vr] = x[i];
    }
    return fmi2OK;
#endif
}

fmi2Status fmiGetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiGetEventIndicators", modelInitialized|modelStepping|modelTerminated))
        return fmi2Error;
    if (invalidNumber(comp, "fmiGetEventIndicators", "ni", ni, NUMBER_OF_EVENT_INDICATORS))
        return fmi2Error;
#if NUMBER_OF_EVENT_INDICATORS>0
    for (i = 0; i < ni; i++) {
        eventIndicators[i] = getEventIndicator(comp, i); // to be implemented by the includer of this file
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiGetEventIndicators: z%d = %.16g", i, eventIndicators[i])
    }
#endif
    return fmi2OK;
}

fmi2Status fmiGetContinuousStates(fmi2Component c, fmi2Real states[], size_t nx) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiGetContinuousStates", modelInitialized|modelStepping|modelTerminated))
        return fmi2Error;
    if (invalidNumber(comp, "fmiGetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;
    if (nullPointer(comp, "fmiGetContinuousStates", "states[]", states))
        return fmi2Error;

#ifdef HAVE_MODELDESCRIPTION_STRUCT
    return generated_fmi2GetContinuousStates(&comp->s.md, vrStates, nx, states);
#else
    for (i = 0; i < nx; i++) {
        fmi2ValueReference vr = vrStates[i];
        states[i] = comp->s.r[vr];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiGetContinuousStates: #r%u# = %.16g", vr, states[i])
    }
    return fmi2OK;
#endif
}

/* Evaluation of the model equations */
fmi2Status fmiGetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx) {
    int i;
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiGetDerivatives", modelInitialized|modelStepping|modelTerminated))
        return fmi2Error;
    if (invalidNumber(comp, "fmiGetDerivatives", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;
    if (nullPointer(comp, "fmiGetDerivatives", "derivatives[]", derivatives))
        return fmi2Error;
#ifdef HAVE_MODELDESCRIPTION_STRUCT
    return generated_fmi2GetDerivatives(&comp->s.md, vrStates, nx, derivatives);
#else
    for (i = 0; i < nx; i++) {
        fmi2ValueReference vr = vrStates[i] + 1;
        derivatives[i] = comp->s.r[vr];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiGetDerivatives: #r%d# = %.16g", vr, derivatives[i])
    }
    return fmi2OK;
#endif
}

fmi2Status fmiGetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiGetNominalContinuousStates", modelInstantiated|modelInitialized|modelStepping|modelTerminated))
        return fmi2Error;
    if (invalidNumber(comp, "fmiGetNominalContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;
    if (nullPointer(comp, "fmiGetNominalContinuousStates", "x_nominal[]", x_nominal))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiGetNominalContinuousStates: x_nominal[0..%d] = 1.0", nx-1)
    for (i = 0; i < nx; i++)
        x_nominal[i] = 1;
    return fmi2OK;
}
#endif // Model Exchange
