#ifndef model_h
#define model_h

#include "fmi.h"

// Our 1D particle model struct
typedef struct {
    fmiReal    x;
    fmiReal    v;
    fmiReal    f;
    fmiReal    amplitude;
    fmiReal    lambda;
    fmiReal    invMass;
    fmiReal    time;
    fmiBoolean initializationMode;
    fmiString instanceName;
    fmiString GUID;
    const fmiCallbackFunctions* functions;
} SlaveInstance;

#endif
