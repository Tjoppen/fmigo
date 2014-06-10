#include "fmi.h"
#include "model.h"

const char * fmiGetTypesPlatform(){
    return fmiTypesPlatform;
}

const char* fmiGetVersion(){
    return fmiVersion;
}

fmiStatus fmiSetDebugLogging(   fmiComponent c,
                                fmiBoolean loggingOn,
                                size_t nCategories,
                                const fmiString categories[]){
    return fmiOK;
}

fmiComponent fmiInstantiate(fmiString instanceName,
                            fmiType fmuType,
                            fmiString fmuGUID,
                            fmiString fmuResourceLocation,
                            const fmiCallbackFunctions* functions,
                            fmiBoolean visible,
                            fmiBoolean loggingOn){

    // Allocate struct
    SlaveInstance* s = functions->allocateMemory(sizeof(SlaveInstance),1);

    // Set members
    s->functions = functions;
    s->initializationMode = fmiFalse;
    s->instanceName = instanceName;
    s->GUID = fmuGUID;

    fmiReset((fmiComponent*)s);

    return (fmiComponent*)s;
}

void fmiFreeInstance(fmiComponent c){
    SlaveInstance* s = (SlaveInstance *)c;
    s->functions->freeMemory(s);
}

fmiStatus fmiSetupExperiment(   fmiComponent c,
                                fmiBoolean toleranceDefined,
                                fmiReal tolerance,
                                fmiReal startTime,
                                fmiBoolean stopTimeDefined,
                                fmiReal stopTime){
    return fmiOK;
}

fmiStatus fmiEnterInitializationMode(fmiComponent c){
    SlaveInstance* s = (SlaveInstance*)c;
    s->initializationMode = fmiTrue;
    return fmiOK;
}

fmiStatus fmiExitInitializationMode(fmiComponent c){
    SlaveInstance* s = (SlaveInstance*)c;
    s->initializationMode = fmiFalse;
    return fmiOK;
}

fmiStatus fmiTerminate(fmiComponent c){
    // Nothing to do here
    return fmiOK;
}

fmiStatus fmiReset(fmiComponent c){
    SlaveInstance* s = (SlaveInstance*)c;
    s->time = 0.0;
    s->x = 0.0;
    s->v = 0.0;
    s->f = 0.0;
    s->invMass = 1.0;
    return fmiOK;
}

fmiStatus fmiGetReal(   fmiComponent c,
                        const fmiValueReference vr[],
                        size_t nvr,
                        fmiReal value[]){
    SlaveInstance* s = (SlaveInstance*)c;
    fmiReal val = 0;
    int i;
    for(i=0; i<nvr; i++){
        switch(vr[i]) {
            case VR_X:  val = s->x; break;
            case VR_V:  val = s->v; break;
            case VR_F:  val = s->f; break;
            case VR_AMPLITUDE:  val = s->amplitude; break;
            default:
                val = 0;
                printf("fmiGetReal unknown VR: %d\n",vr[i]);
                break;
        }
        value[i] = val;
    }
    return fmiOK;
}

fmiStatus fmiGetInteger(fmiComponent c,
                        const fmiValueReference vr[],
                        size_t nvr,
                        fmiInteger value[]){
    // No integers
    return fmiOK;
}

fmiStatus fmiGetBoolean(fmiComponent c,
                        const fmiValueReference vr[],
                        size_t nvr,
                        fmiBoolean value[]){
    // no booleans
    return fmiOK;
}

fmiStatus fmiGetString (fmiComponent c,
                        const fmiValueReference vr[],
                        size_t nvr,
                        fmiString value[]){
    // No strings
    return fmiOK;
}

fmiStatus fmiSetReal(   fmiComponent c,
                        const fmiValueReference vr[],
                        size_t nvr,
                        const fmiReal value[]){
    SlaveInstance* s = (SlaveInstance*)c;
    fmiReal val = 0;
    int i;
    for(i=0; i<nvr; i++){
        val = value[i];
        switch(vr[i]) {
            case VR_X:  s->x = val; break;
            case VR_V:  s->v = val; break;
            case VR_F:  s->f = val; break;
            case VR_AMPLITUDE:  s->amplitude = val; break;
        }
    }
    return fmiOK;
}

fmiStatus fmiSetInteger(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiInteger value[]){
    return fmiOK;
}

fmiStatus fmiSetBoolean(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiBoolean value[]){
    return fmiOK;
}

fmiStatus fmiSetString (fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiString value[]){
    return fmiOK;
}

// Internal function
void copySlaveInstance(const SlaveInstance * from, SlaveInstance * to){
    to->functions =           from->functions;
    to->initializationMode =  from->initializationMode;
    to->instanceName =        from->instanceName;
    to->GUID =                from->GUID;
    to->time =                from->time;
    to->invMass =             from->invMass;
    to->x =                   from->x;
    to->v =                   from->v;
    to->f =                   from->f;
    to->lambda =              from->lambda;
    to->amplitude =           from->amplitude;
}

// Makes a copy of the internal state and returns a pointer to it
fmiStatus fmiGetFMUstate (fmiComponent c, fmiFMUstate* FMUstate){
    SlaveInstance* s = (SlaveInstance*)c;

    // Allocate struct
    SlaveInstance* copy = s->functions->allocateMemory(sizeof(SlaveInstance),1);

    // Set members
    copySlaveInstance(s,copy);

    *FMUstate = copy;

    return fmiOK;
}

fmiStatus fmiSetFMUstate (fmiComponent c, fmiFMUstate FMUstate){
    SlaveInstance* s = (SlaveInstance*)c;
    SlaveInstance* state = (SlaveInstance*)FMUstate;
    copySlaveInstance(state,s);
    return fmiOK;
}

fmiStatus fmiFreeFMUstate(fmiComponent c, fmiFMUstate* FMUstate){
    SlaveInstance* s = (SlaveInstance*)c;
    s->functions->freeMemory(FMUstate);
    return fmiOK;
}


// These are TODO
fmiStatus fmiSerializedFMUstateSize(fmiComponent c, fmiFMUstate FMUstate, size_t *size){
    return fmiOK;
}
fmiStatus fmiSerializeFMUstate(fmiComponent c, fmiFMUstate FMUstate,fmiByte serializedState[], size_t size){
    return fmiOK;
}
fmiStatus fmiDeSerializeFMUstate(fmiComponent c,const fmiByte serializedState[],size_t size, fmiFMUstate* FMUstate){
    return fmiOK;
}


fmiStatus fmiGetDirectionalDerivative(  fmiComponent c,
                                        const fmiValueReference vUnknown_ref[],
                                        size_t nUnknown,
                                        const fmiValueReference vKnown_ref[],
                                        size_t nKnown,
                                        const fmiReal dvKnown[],
                                        fmiReal dvUnknown[]){
    SlaveInstance * s = (SlaveInstance*)c;

    // Set all to zero
    int i;
    for(i=0; i<nUnknown; i++)
        dvUnknown[i] = 0;

    if(nKnown > 0 && nUnknown > 0){
        if(vKnown_ref[0] == VR_F && vUnknown_ref[0] == VR_V){
            SlaveInstance temp;
            copySlaveInstance(s,&temp);
            temp.f = dvKnown[0];
            step(&temp,1.0,fmiFalse); // use dt = 1, then the scaling will be ok?
            dvUnknown[0] = temp.v;
        }
    }

    return fmiOK;
}

fmiStatus fmiSetRealInputDerivatives(   fmiComponent c,
                                        const fmiValueReference vr[],
                                        size_t nvr,
                                        const fmiInteger order[],
                                        const fmiReal value[]){

}
fmiStatus fmiGetRealOutputDerivatives ( fmiComponent c,
                                        const fmiValueReference vr[],
                                        size_t nvr,
                                        const fmiInteger order[],
                                        fmiReal value[]){

}

fmiStatus fmiDoStep(fmiComponent c,
                    fmiReal currentCommunicationPoint,
                    fmiReal communicationStepSize,
                    fmiBoolean noSetFMUStatePriorToCurrentPoint){
    SlaveInstance * s = (SlaveInstance*)c;
    step(s,communicationStepSize,fmiTrue);
    s->time = currentCommunicationPoint + communicationStepSize;

    printf("x=%g\n", s->x);

    return fmiOK;
}

fmiStatus fmiCancelStep(fmiComponent c){
    return fmiOK;
}

fmiStatus fmiGetStatus(fmiComponent c, const fmiStatusKind s, fmiStatus* value){
    return fmiOK;
}

fmiStatus fmiGetIntegerStatus(fmiComponent c, const fmiStatusKind s, fmiInteger* value){
    return fmiOK;
}

fmiStatus fmiGetBooleanStatus(fmiComponent c, const fmiStatusKind s, fmiBoolean* value){
    return fmiOK;
}

fmiStatus fmiGetStringStatus (fmiComponent c, const fmiStatusKind s, fmiString* value){
    return fmiOK;
}

