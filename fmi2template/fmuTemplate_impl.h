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
        comp->state = modelError;
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Invalid argument %s = %d. Expected %d.", f, arg, n, nExpected)
        return fmi2True;
    }
    return fmi2False;
}

static fmi2Boolean invalidState(ModelInstance *comp, const char *f, int statesExpected) {
    if (!comp)
        return fmi2True;
    if (!(comp->state & statesExpected)) {
        comp->state = modelError;
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Illegal call sequence.", f)
        return fmi2True;
    }
    return fmi2False;
}

static fmi2Boolean nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p) {
    if (!p) {
        comp->state = modelError;
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Invalid argument %s = NULL.", f, arg)
        return fmi2True;
    }
    return fmi2False;
}

static fmi2Boolean vrOutOfRange(ModelInstance *comp, const char *f, fmi2ValueReference vr, int end) {
    if (vr >= end) {
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Illegal value reference %u.", f, vr)
        comp->state = modelError;
        return fmi2True;
    }
    return fmi2False;
}

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

// ---------------------------------------------------------------------------
// FMI functions
// ---------------------------------------------------------------------------
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
        comp->GUID = functions->allocateMemory(1 + strlen(fmuGUID), sizeof(char));

        // set all categories to on or off. fmi2SetDebugLogging should be called to choose specific categories.
        for (i = 0; i < NUMBER_OF_CATEGORIES; i++) {
            comp->logCategories[i] = loggingOn;
        }
    }
    if (!comp || !comp->instanceName || !comp->GUID) {

        functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
            "fmi2Instantiate: Out of memory.");
        return NULL;
    }
    strcpy(comp->instanceName, instanceName);
    comp->type = fmuType;
    strcpy(comp->GUID, fmuGUID);
    comp->functions = functions;
    comp->componentEnvironment = functions->componentEnvironment;
    comp->loggingOn = loggingOn;
    comp->state = modelInstantiated;
    setStartValues(comp); // to be implemented by the includer of this file

    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Instantiate: GUID=%s", fmuGUID)

    return comp;
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance,
                            fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {

    // ignore arguments: stopTimeDefined, stopTime
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetupExperiment", modelInstantiated))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetupExperiment: toleranceDefined=%d tolerance=%g",
        toleranceDefined, tolerance)

    comp->time = startTime;
    return fmi2OK;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2EnterInitializationMode", modelInstantiated))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2EnterInitializationMode")

    comp->state = modelInitializationMode;
    return fmi2OK;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2ExitInitializationMode", modelInitializationMode))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2ExitInitializationMode")

    initialize(comp, &comp->eventInfo); // to be implemented by the includer of this file
    comp->state = modelInitialized;
    return fmi2OK;
}

fmi2Status fmi2Terminate(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2Terminate", modelInitialized|modelStepping))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Terminate")

    comp->state = modelTerminated;
    return fmi2OK;
}

fmi2Status fmi2Reset(fmi2Component c) {
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2Reset", modelInitialized|modelStepping))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Reset")

    comp->state = modelInstantiated;
    setStartValues(comp); // to be implemented by the includer of this file
    return fmi2OK;
}

void fmi2FreeInstance(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (!comp) return;
    if (invalidState(comp, "fmi2FreeInstance", modelTerminated))
        return;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2FreeInstance")

    {
        int i;
        for (i = 0; i < NUMBER_OF_STRINGS; i++){
            if (comp->s[i]) comp->functions->freeMemory(comp->s[i]);
        }
    }
    if (comp->instanceName) comp->functions->freeMemory(comp->instanceName);
    if (comp->GUID) comp->functions->freeMemory(comp->GUID);
    comp->functions->freeMemory(comp);
}

// ---------------------------------------------------------------------------
// FMI functions: class methods not depending of a specific model instance
// ---------------------------------------------------------------------------

const char* fmi2GetVersion() {
    return fmi2Version;
}

const char* fmi2GetTypesPlatform() {
    return fmi2TypesPlatform;
}

// ---------------------------------------------------------------------------
// FMI functions: logging control, setters and getters for Real, Integer,
// Boolean, String
// ---------------------------------------------------------------------------

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

fmi2Status fmi2GetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetReal", modelInitializationMode|modelInitialized|modelStepping|modelError))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetReal", "vr[]", vr))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetReal", "value[]", value))
        return fmi2Error;
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2GetReal", vr[i], NUMBER_OF_REALS))
            return fmi2Error;
        value[i] = getReal(comp, vr[i]); // to be implemented by the includer of this file

        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetReal: #r%u# = %.16g", vr[i], value[i])
    }
    return fmi2OK;
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetInteger", modelInitializationMode|modelInitialized|modelStepping|modelError))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetInteger", "vr[]", vr))
            return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetInteger", "value[]", value))
            return fmi2Error;
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2GetInteger", vr[i], NUMBER_OF_INTEGERS))
            return fmi2Error;
        value[i] = comp->i[vr[i]];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetInteger: #i%u# = %d", vr[i], value[i])
    }
    return fmi2OK;
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetBoolean", modelInitializationMode|modelInitialized|modelStepping|modelError))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetBoolean", "vr[]", vr))
            return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetBoolean", "value[]", value))
            return fmi2Error;
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2GetBoolean", vr[i], NUMBER_OF_BOOLEANS))
            return fmi2Error;
        value[i] = comp->b[vr[i]];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetBoolean: #b%u# = %s", vr[i], value[i]? "true" : "false")
    }
    return fmi2OK;
}

fmi2Status fmi2GetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetString", modelInitializationMode|modelInitialized|modelStepping|modelError))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2GetString", "vr[]", vr))
            return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2GetString", "value[]", value))
            return fmi2Error;
    for (i=0; i<nvr; i++) {
        if (vrOutOfRange(comp, "fmi2GetString", vr[i], NUMBER_OF_STRINGS))
            return fmi2Error;
        value[i] = comp->s[vr[i]];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetString: #s%u# = '%s'", vr[i], value[i])
    }
    return fmi2OK;
}

fmi2Status fmi2SetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetReal", modelInstantiated|modelInitializationMode|modelInitialized|modelStepping))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetReal", "vr[]", vr))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetReal", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetReal: nvr = %d", nvr)
    // no check whether setting the value is allowed in the current state
    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2SetReal", vr[i], NUMBER_OF_REALS))
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetReal: #r%d# = %.16g", vr[i], value[i])
        comp->r[vr[i]] = value[i];
    }
    return fmi2OK;
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetInteger", modelInstantiated|modelInitializationMode|modelInitialized|modelStepping))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetInteger", "vr[]", vr))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetInteger", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetInteger: nvr = %d", nvr)

    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2SetInteger", vr[i], NUMBER_OF_INTEGERS))
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetInteger: #i%d# = %d", vr[i], value[i])
        comp->i[vr[i]] = value[i];
    }
    return fmi2OK;
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetBoolean", modelInstantiated|modelInitializationMode|modelInitialized|modelStepping))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2SetBoolean", "vr[]", vr))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2SetBoolean", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetBoolean: nvr = %d", nvr)

    for (i = 0; i < nvr; i++) {
        if (vrOutOfRange(comp, "fmi2SetBoolean", vr[i], NUMBER_OF_BOOLEANS))
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetBoolean: #b%d# = %s", vr[i], value[i] ? "true" : "false")
        comp->b[vr[i]] = value[i];
    }
    return fmi2OK;
}

fmi2Status fmi2SetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    int i, n;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetString", modelInstantiated|modelInitializationMode|modelInitialized|modelStepping))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2SetString", "vr[]", vr))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2SetString", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetString: nvr = %d", nvr)

    for (i = 0; i < nvr; i++) {
        char *string = comp->s[vr[i]];
        if (vrOutOfRange(comp, "fmi2SetString", vr[i], NUMBER_OF_STRINGS))
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetString: #s%d# = '%s'", vr[i], value[i])

        if (nullPointer(comp, "fmi2SetString", "value[i]", value[i]))
            return fmi2Error;
        if (string == NULL || strlen(string) < strlen(value[i])) {
            if (string) comp->functions->freeMemory(string);
            comp->s[vr[i]] = comp->functions->allocateMemory(1 + strlen(value[i]), sizeof(char));
            if (!comp->s[vr[i]]) {
                comp->state = modelError;
                FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "fmi2SetString: Out of memory.")
                return fmi2Error;
            }
        }
        strcpy(comp->s[vr[i]], value[i]);
    }
    return fmi2OK;
}

fmi2Status fmi2GetFMUstate (fmi2Component c, fmi2FMUstate* FMUstate) {
    return unsupportedFunction(c, "fmi2GetFMUstate",
        modelInstantiated|modelInitializationMode|modelInitialized|modelStepping|modelTerminated|modelError);
}
fmi2Status fmi2SetFMUstate (fmi2Component c, fmi2FMUstate FMUstate) {
    return unsupportedFunction(c, "fmi2SetFMUstate",
        modelInstantiated|modelInitializationMode|modelInitialized|modelStepping|modelTerminated|modelError);
}
fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
    return unsupportedFunction(c, "fmi2FreeFMUstate",
        modelInstantiated|modelInitializationMode|modelInitialized|modelStepping|modelTerminated|modelError);
}
fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t *size) {
    return unsupportedFunction(c, "fmi2SerializedFMUstateSize",
        modelInstantiated|modelInitializationMode|modelInitialized|modelStepping|modelTerminated|modelError);
}
fmi2Status fmi2SerializeFMUstate (fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size) {
    return unsupportedFunction(c, "fmi2SerializeFMUstate",
        modelInstantiated|modelInitializationMode|modelInitialized|modelStepping|modelTerminated|modelError);
}
fmi2Status fmi2DeSerializeFMUstate (fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
    return unsupportedFunction(c, "fmi2DeSerializeFMUstate",
        modelInstantiated|modelInitializationMode|modelInitialized|modelStepping|modelTerminated|modelError);
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
        *value = comp->time;
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
        *value = comp->eventInfo.terminateSimulation;
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

    if (comp->state == modelStepping) {
        comp->eventInfo.newDiscreteStatesNeeded = fmi2False;
        comp->eventInfo.terminateSimulation = fmi2False;
        comp->eventInfo.nominalsOfContinuousStatesChanged = fmi2False;
        comp->eventInfo.valuesOfContinuousStatesChanged = fmi2False;
        comp->eventInfo.nextEventTimeDefined = fmi2False;
        comp->eventInfo.nextEventTime = 0; // next time event if nextEventTimeDefined = fmi2True

        eventUpdate(comp, &comp->eventInfo);
    }

    // model in stepping state
    comp->state = modelStepping;

    // copy internal eventInfo of component to output eventInfo
    eventInfo->newDiscreteStatesNeeded = comp->eventInfo.newDiscreteStatesNeeded;
    eventInfo->terminateSimulation = comp->eventInfo.terminateSimulation;
    eventInfo->nominalsOfContinuousStatesChanged = comp->eventInfo.nominalsOfContinuousStatesChanged;
    eventInfo->valuesOfContinuousStatesChanged = comp->eventInfo.valuesOfContinuousStatesChanged;
    eventInfo->nextEventTimeDefined = comp->eventInfo.nextEventTimeDefined;
    eventInfo->nextEventTime = comp->eventInfo.nextEventTime;

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
    comp->time = time;
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
    for (i = 0; i < nx; i++) {
        fmi2ValueReference vr = vrStates[i];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiSetContinuousStates: #r%d#=%.16g", vr, x[i])
        assert(vr >= 0 && vr < NUMBER_OF_REALS);
        comp->r[vr] = x[i];
    }
    return fmi2OK;
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
#if NUMBER_OF_STATES>0
    for (i = 0; i < nx; i++) {
        fmi2ValueReference vr = vrStates[i] + 1;
        derivatives[i] = getReal(comp, vr); // to be implemented by the includer of this file
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiGetDerivatives: #r%d# = %.16g", vr, derivatives[i])
    }
#endif
    return fmi2OK;
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
    for (i = 0; i < nx; i++) {
        fmi2ValueReference vr = vrStates[i];
        states[i] = getReal(comp, vr); // to be implemented by the includer of this file
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiGetContinuousStates: #r%u# = %.16g", vr, states[i])
    }
    return fmi2OK;
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
