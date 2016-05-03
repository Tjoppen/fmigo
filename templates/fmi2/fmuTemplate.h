/* ---------------------------------------------------------------------------*
 * fmuTemplate.h
 * Definitions by the includer of this file
 * Copyright QTronic GmbH. All rights reserved.
 * ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "FMI2/fmi2Functions.h"

// categories of logging supported by model.
// Value is the index in logCategories of a ModelInstance.
#define LOG_ALL       0
#define LOG_ERROR     1
#define LOG_FMI_CALL  2
#define LOG_EVENT     3

#define NUMBER_OF_CATEGORIES 4

typedef enum {
    modelInstantiated       = 1<<0,
    modelInitializationMode = 1<<1,
    modelInitialized        = 1<<2, // state just after fmiExitInitializationMode
    modelStepping           = 1<<3, // state after initialization
    modelTerminated         = 1<<4,
    modelError              = 1<<5
} ModelState;

#ifndef WIN32
//Tomas: needed since fmuTemplate_impl.h doesn't define it
static inline int max(int a, int b) {
    return a > b ? a : b;
}
#endif

//making a special macro for this because reasons
//needed because cl.exe doesn't accept zero-length arrays
#define atleast1(a) ((a) < 1 ? 1 : (a))

typedef struct {
    char* instanceName;
    const fmi2CallbackFunctions *functions;
    fmi2ComponentEnvironment componentEnvironment;
    fmi2Type type;
    fmi2Boolean loggingOn;
    fmi2Boolean logCategories[NUMBER_OF_CATEGORIES];

    struct state_t {
#ifdef HAVE_MODELDESCRIPTION_STRUCT
        modelDescription_t md;
#else
        fmi2Real    r[atleast1(NUMBER_OF_REALS)];
        fmi2Integer i[atleast1(NUMBER_OF_INTEGERS)];
        fmi2Boolean b[atleast1(NUMBER_OF_BOOLEANS)];
        fmi2Boolean isPositive[atleast1(NUMBER_OF_EVENT_INDICATORS)];
#endif
#ifndef CONSOLE
        fmi2Real time;
        ModelState state;
        fmi2EventInfo eventInfo;
#endif
#ifdef SIMULATION_TYPE
        SIMULATION_TYPE simulation;  /* this contains everything in the
                                          simulation not related the FMI */
#endif
    } s;
} ModelInstance;

typedef struct state_t state_t;
