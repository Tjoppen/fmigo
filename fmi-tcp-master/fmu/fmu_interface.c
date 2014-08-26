#include <stdlib.h>
#include "include/fmu_interface.h"

/***************************************************
Common Functions
****************************************************/

const char* fmiGetTypesPlatform() {
  return fmiTypesPlatform;
}

const char* fmiGetVersion() {
  return fmiVersion;
}

fmiStatus fmiSetDebugLogging(fmiComponent c, fmiBoolean loggingOn, size_t nCategories, const fmiString categories[]) {
  // TODO Write code here
  return fmiOK;
}

fmiComponent fmiInstantiate(fmiString instanceName, fmiType fmuType, fmiString fmuGUID, fmiString fmuResourceLocation, const fmiCallbackFunctions* functions,
    fmiBoolean visible, fmiBoolean loggingOn) {
  // TODO Write code here
  return NULL;
}

void fmiFreeInstance(fmiComponent c) {

}

fmiStatus fmiSetupExperiment(fmiComponent c, fmiBoolean toleranceDefined, fmiReal tolerance, fmiReal startTime, fmiBoolean stopTimeDefined, fmiReal stopTime) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiEnterInitializationMode(fmiComponent c) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiExitInitializationMode(fmiComponent c) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiTerminate(fmiComponent c) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiReset(fmiComponent c) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetReal(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiReal value[]) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetInteger(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiInteger value[]) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetBoolean(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiBoolean value[]){
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetString(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiString value[]) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiSetReal(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiReal value[]) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiSetInteger(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiInteger value[]) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiSetBoolean(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiBoolean value[]) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiSetString(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiString value[]) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetFMUstate(fmiComponent c, fmiFMUstate* FMUstate) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiSetFMUstate(fmiComponent c, fmiFMUstate FMUstate) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiFreeFMUstate(fmiComponent c, fmiFMUstate* FMUstate) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiSerializedFMUstateSize(fmiComponent c, fmiFMUstate FMUstate, size_t *size) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiSerializeFMUstate(fmiComponent c, fmiFMUstate FMUstate, fmiByte serializedState[], size_t size) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiDeSerializeFMUstate(fmiComponent c, const fmiByte serializedState[], size_t size, fmiFMUstate* FMUstate) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetDirectionalDerivative(fmiComponent c, const fmiValueReference vUnknown_ref[], size_t nUnknown, const fmiValueReference vKnown_ref[] , size_t nKnown,
    const fmiReal dvKnown[], fmiReal dvUnknown[]) {
  // TODO Write code here
  return fmiOK;
}

/***************************************************
Functions for FMI for Model Exchange
****************************************************/

/***************************************************
Functions for FMI for Co-Simulation
****************************************************/
fmiStatus fmiSetRealInputDerivatives(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiInteger order[], const fmiReal value[]) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetRealOutputDerivatives (fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiInteger order[], fmiReal value[]) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiDoStep(fmiComponent c, fmiReal currentCommunicationPoint, fmiReal communicationStepSize, fmiBoolean noSetFMUStatePriorToCurrentPoint) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiCancelStep(fmiComponent c) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetStatus(fmiComponent c, const fmiStatusKind s, fmiStatus* value) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetRealStatus(fmiComponent c, const fmiStatusKind s, fmiReal* value) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetIntegerStatus(fmiComponent c, const fmiStatusKind s, fmiInteger* value) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetBooleanStatus(fmiComponent c, const fmiStatusKind s, fmiBoolean* value) {
  // TODO Write code here
  return fmiOK;
}

fmiStatus fmiGetStringStatus(fmiComponent c, const fmiStatusKind s, fmiString* value) {
  // TODO Write code here
  return fmiOK;
}
