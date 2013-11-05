#include "fmi.h"

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
    SlaveInstance* c = functions->allocateMemory(sizeof(SlaveInstance),1);
    c->functions = functions;
    return (fmiComponent*)c;
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
    return fmiOK;
}

fmiStatus fmiExitInitializationMode(fmiComponent c){
    return fmiOK;
}

fmiStatus fmiTerminate(fmiComponent c){
    return fmiOK;
}

fmiStatus fmiReset(fmiComponent c){
    return fmiOK;
}

fmiStatus fmiGetReal(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiReal value[]){
    return fmiOK;
}

fmiStatus fmiGetInteger(fmiComponent c,
                        const fmiValueReference vr[],
                        size_t nvr,
                        fmiInteger value[]){
    return fmiOK;
}

fmiStatus fmiGetBoolean(fmiComponent c,
                        const fmiValueReference vr[],
                        size_t nvr,
                        fmiBoolean value[]){
    return fmiOK;
}

fmiStatus fmiGetString (fmiComponent c,
                        const fmiValueReference vr[],
                        size_t nvr,
                        fmiString value[]){
    return fmiOK;
}

fmiStatus fmiSetReal(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiReal value[]){
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

fmiStatus fmiGetFMUstate (fmiComponent c, fmiFMUstate* FMUstate){
    return fmiOK;
}

fmiStatus fmiSetFMUstate (fmiComponent c, fmiFMUstate FMUstate){
    return fmiOK;
}

fmiStatus fmiFreeFMUstate(fmiComponent c, fmiFMUstate* FMUstate){
    return fmiOK;
}
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

