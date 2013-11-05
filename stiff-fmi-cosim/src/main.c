#include "fmi.h"

void loggerFunction(fmiComponentEnvironment componentEnvironment,
                    fmiString instanceName,
                    fmiStatus status,
                    fmiString category,
                    fmiString message, ...){
    printf("%s",message);
}

int main(){
    fmiCallbackFunctions cbf;
    cbf.logger = loggerFunction; //logger function
    cbf.allocateMemory = calloc;
    cbf.freeMemory = free;
    cbf.stepFinished = NULL; //synchronous execution
    cbf.componentEnvironment = NULL;

    // Instantiate both slaves
    fmiComponent s1 = fmiInstantiate("Tool1" , fmiCoSimulation, "GUID1", "", &cbf, fmiTrue, fmiTrue);
    fmiComponent s2 = fmiInstantiate("Tool2" , fmiCoSimulation, "GUID2", "", &cbf, fmiTrue, fmiTrue);

    if ((s1 == NULL) || (s2 == NULL))
        return 1;

    // Start and stop time
    fmiReal startTime = 0;
    fmiReal stopTime =  1;

    // communication step size
    fmiReal h = 0.1;

    // set all variable start values (of "ScalarVariable / <type> / start")
    //fmiSetReal/Integer/Boolean/String(s1, ...);
    //fmiSetReal/Integer/Boolean/String(s2, ...);

    fmiSetupExperiment(s1, fmiFalse, 0.0, startTime, fmiTrue, stopTime);
    fmiSetupExperiment(s1, fmiFalse, 0.0, startTime, fmiTrue, stopTime);

    fmiEnterInitializationMode(s1);
    fmiEnterInitializationMode(s2);

    // set the input values at time = startTime
    //fmiSetReal/Integer/Boolean/String(s1, ...);
    //fmiSetReal/Integer/Boolean/String(s2, ...);
    //fmiExitInitializationMode(s1);
    //fmiExitInitializationMode(s2);

    fmiReal tc = startTime; //Current master time
    fmiStatus status = fmiOK;
    fmiBoolean boolVal, terminateSimulation = fmiFalse;


    while ((tc < stopTime) && (status == fmiOK)) {

        printf("t = %g\n",tc);

        // retrieve outputs
        //fmiGetReal(s1, ..., 1, &y1);
        //fmiGetReal(s2, ..., 1, &y2);

        // set inputs
        //fmiSetReal(s1, ..., 1, &y2);
        //fmiSetReal(s2, ..., 1, &y1);

        //call slave s1 and check status
        status = fmiDoStep(s1, tc, h, fmiTrue);
        switch (status) {
        case fmiDiscard:
            fmiGetBooleanStatus(s1, fmiTerminated, &boolVal);
            if (boolVal == fmiTrue)
                printf("Slave s1 wants to terminate simulation.");
        case fmiError:
        case fmiFatal:
            terminateSimulation = fmiTrue;
            break;
        }

        if (terminateSimulation)
            break;

        // call slave s2 and check status as above
        status = fmiDoStep(s2, tc, h, fmiTrue);

        // increment master time
        tc += h;
    }

    // Shutdown sub-phase
    if ((status != fmiError) && (status != fmiFatal)) {
        fmiTerminate(s1);
        fmiTerminate(s2);
    }

    if (status != fmiFatal){
        fmiFreeInstance(s1);
        fmiFreeInstance(s2);
    }

    return 0;
}
