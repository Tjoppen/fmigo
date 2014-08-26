/*
    Copyright (C) 2012 Modelon AB

    This program is free software: you can redistribute it and/or modify
    it under the terms of the BSD style license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    FMILIB_License.txt file for more details.

    You should have received a copy of the FMILIB_License.txt file
    along with this program. If not, contact Modelon AB <http://www.modelon.com>.
*/

#include <assert.h>
#include <FMI2/fmi2_capi.h>
#include <FMI2/fmi2_capi_impl.h>


fmi2_status_t fmi2_capi_enter_event_mode(fmi2_capi_t* fmu)
{
    assert(fmu); assert(fmu->c);
    jm_log_verbose(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiEnterEventMode");
    return fmu->fmiEnterEventMode(fmu->c);
}

fmi2_status_t fmi2_capi_new_discrete_states(fmi2_capi_t* fmu, fmi2_event_info_t* eventInfo)
{
    assert(fmu); assert(fmu->c);
    jm_log_verbose(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiNewDiscreteStates");
    return fmu->fmiNewDiscreteStates(fmu->c, eventInfo);
}

fmi2_status_t fmi2_capi_enter_continuous_time_mode(fmi2_capi_t* fmu)
{
    assert(fmu); assert(fmu->c);
    jm_log_verbose(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiEnterContinuousTimeMode");
    return fmu->fmiEnterContinuousTimeMode(fmu->c);
}

fmi2_status_t fmi2_capi_set_time(fmi2_capi_t* fmu, fmi2_real_t time)
{
	assert(fmu);
	jm_log_debug(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiGetModelTypesPlatform");
	return fmu->fmiSetTime(fmu->c, time);
}

fmi2_status_t fmi2_capi_set_continuous_states(fmi2_capi_t* fmu, const fmi2_real_t x[], size_t nx)
{
	assert(fmu);
	jm_log_debug(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiSetContinuousStates");
	return fmu->fmiSetContinuousStates(fmu->c, x, nx);
}

fmi2_status_t fmi2_capi_completed_integrator_step(fmi2_capi_t* fmu,
  fmi2_boolean_t noSetFMUStatePriorToCurrentPoint,
  fmi2_boolean_t* enterEventMode, fmi2_boolean_t* terminateSimulation)
{
    assert(fmu);
    jm_log_debug(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiCompletedIntegratorStep");
    return fmu->fmiCompletedIntegratorStep(fmu->c, noSetFMUStatePriorToCurrentPoint,
                                           enterEventMode, terminateSimulation);
}

fmi2_status_t fmi2_capi_get_derivatives(fmi2_capi_t* fmu, fmi2_real_t derivatives[], size_t nx)
{
	assert(fmu);
	jm_log_debug(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiGetDerivatives");
	return fmu->fmiGetDerivatives(fmu->c, derivatives, nx);
}

fmi2_status_t fmi2_capi_get_event_indicators(fmi2_capi_t* fmu, fmi2_real_t eventIndicators[], size_t ni)
{
	assert(fmu);
	jm_log_debug(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiGetEventIndicators");
	return fmu->fmiGetEventIndicators(fmu->c, eventIndicators, ni);
}

fmi2_status_t fmi2_capi_get_continuous_states(fmi2_capi_t* fmu, fmi2_real_t states[], size_t nx)
{
	assert(fmu);
	jm_log_debug(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiGetContinuousStates");
	return fmu->fmiGetContinuousStates(fmu->c, states, nx);
}

fmi2_status_t fmi2_capi_get_nominals_of_continuous_states(fmi2_capi_t* fmu, fmi2_real_t x_nominal[], size_t nx)
{
	assert(fmu);
	jm_log_debug(fmu->callbacks, FMI_CAPI_MODULE_NAME, "Calling fmiGetNominalsOfContinuousStates");
	return fmu->fmiGetNominalsOfContinuousStates(fmu->c, x_nominal, nx);
}
