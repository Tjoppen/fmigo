#include "fmi.h"

void logger(fmiComponentEnvironment componentEnvironment,
                    fmiString instanceName,
                    fmiStatus status,
                    fmiString category,
                    fmiString message, ...){
    printf("%s",message);
}

int main(){
    fmiCallbackFunctions cbf;
    cbf.logger = logger; //logger function
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
    fmiReal stopTime =  10;

    // communication step size
    fmiReal h = 0.1;

    // set all variable start values (of "ScalarVariable / <type> / start")

    fmiSetupExperiment(s1, fmiFalse, 0.0, startTime, fmiTrue, stopTime);
    fmiSetupExperiment(s2, fmiFalse, 0.0, startTime, fmiTrue, stopTime);

    fmiEnterInitializationMode(s1);
    fmiEnterInitializationMode(s2);

    // set the input values at time = startTime
    fmiValueReference amp[1] = {VR_AMPLITUDE};
    fmiReal amps[1] = {0};
    fmiSetReal(s1,amp,1,amps);
    amps[0] = 1;
    fmiSetReal(s2,amp,1,amps);

    fmiExitInitializationMode(s1);
    fmiExitInitializationMode(s2);

    fmiReal tc = startTime; //Current master time
    fmiStatus status = fmiOK;
    fmiBoolean boolVal, terminateSimulation = fmiFalse;

    size_t nUnknown = 1;
    size_t nKnown = 1;
    fmiValueReference vUnknown_ref[1] = {VR_V};
    fmiValueReference vKnown_ref[1] = {VR_F};
    fmiReal dvUnknown[1] = {0};
    fmiReal dvKnown[1] = {1};
    fmiValueReference get_velocity_refs[1] = {VR_V};
    fmiReal get_real_vals[1] = {0};
    fmiValueReference get_pos_refs[1] = {VR_X};
    int i,j;
    fmiReal E = 0, d=3;

    fmiFMUstate state1 = NULL;
    fmiFMUstate state2 = NULL;

    while ((tc < stopTime) && (status == fmiOK)) {
        printf("\n=== T=%g ===\n",tc);

        fmiReal a = 4/(1+4*d)/h;
        fmiReal b = 4*d/(1+4*d);

        // Get Z setting input forces = 0 and stepping
        status = fmiGetFMUstate(s1, &state1);
        status = fmiGetFMUstate(s2, &state2);
        status = fmiDoStep(s1, tc, h, fmiTrue);
        status = fmiDoStep(s2, tc, h, fmiTrue);
        fmiGetReal(s1,get_velocity_refs,1,get_real_vals);
        fmiReal Z = get_real_vals[0];
        fmiGetReal(s2,get_velocity_refs,1,get_real_vals);
        Z -= get_real_vals[0];

        // Go back to prev state
        status = fmiSetFMUstate(s1,state1);
        status = fmiSetFMUstate(s2,state2);

        // Get inv(M)*G' by setting input forces to the corresponding jacobian entry
        status = fmiGetDirectionalDerivative(s1, vUnknown_ref, nUnknown, vKnown_ref, nKnown, dvKnown, dvUnknown);
        fmiReal S = dvUnknown[0];
        status = fmiGetDirectionalDerivative(s2, vUnknown_ref, nUnknown, vKnown_ref, nKnown, dvKnown, dvUnknown);
        S += dvUnknown[0];

        // Assemble g and G*W
        fmiGetReal(s1,get_pos_refs,1,get_real_vals);
        fmiReal g = get_real_vals[0];
        fmiGetReal(s2,get_pos_refs,1,get_real_vals);
        g -= get_real_vals[0];
        fmiGetReal(s1,get_velocity_refs,1,get_real_vals);
        fmiReal GW = get_real_vals[0];
        fmiGetReal(s2,get_velocity_refs,1,get_real_vals);
        GW -= get_real_vals[0];

        // Solve constraints
        fmiReal lambda = ( - a*g - b*GW - Z ) / (S + E);

        // Apply master force as external force
        fmiValueReference force_vrs[1] = {VR_F};
        fmiReal force[1] = {lambda};
        fmiSetReal(s1,force_vrs,1,force);
        force[0] = -lambda;
        fmiSetReal(s2,force_vrs,1,force);

        // Step slave systems
        status = fmiDoStep(s1, tc, h, fmiTrue);
        status = fmiDoStep(s2, tc, h, fmiTrue);

        // Increment master time
        tc += h;
    }

    fmiFreeFMUstate(s1, state1);
    fmiFreeFMUstate(s2, state2);

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
