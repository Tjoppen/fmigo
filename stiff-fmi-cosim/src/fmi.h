#include <stdio.h>
#include <string.h>

#include "fmiFunctions.h"

// Our model struct
typedef struct {
    fmiReal    *r;
    fmiInteger *i;
    fmiBoolean *b;
    fmiString  *s;
    fmiBoolean *isPositive;
    fmiReal time;
    fmiString instanceName;
    fmiString GUID;
    const fmiCallbackFunctions* functions;
    fmiBoolean loggingOn;
    fmiFMUstate state;
    fmiEventInfo eventInfo;
} SlaveInstance;
