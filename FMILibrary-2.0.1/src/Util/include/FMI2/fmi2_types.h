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

#ifndef FMI2_TYPES_H_
#define FMI2_TYPES_H_
/** \file fmi2_types.h
	Transformation of the standard FMI type names into fmi2_ prefixed.
*/
/**
	\addtogroup jm_utils
	@{
		\addtogroup fmi2_utils
	@}
*/

/* Standard FMI 2.0 types */
#include <FMI2/fmi2TypesPlatform.h>

/* Tomas: typedef, don't #define and #undef >:( */
/**	\addtogroup fmi2_utils Functions and types supporting FMI 2.0 processing.
	@{
*/
typedef fmi2Component fmi2_component_t;
typedef fmi2ComponentEnvironment fmi2_component_environment_t;
typedef fmi2FMUstate fmi2_FMU_state_t;
typedef fmi2ValueReference fmi2_value_reference_t;
typedef fmi2Real fmi2_real_t;
typedef fmi2Integer fmi2_integer_t;
typedef fmi2Boolean fmi2_boolean_t;
typedef fmi2Char fmi2_char_t;
typedef fmi2String fmi2_string_t;
typedef fmi2Byte fmi2_byte_t;
/** @}*/

/** FMI platform name constant string.*/
static const char * fmi2_get_types_platform(void) {
	return fmi2TypesPlatform;
}

/** FMI boolean constants.*/
typedef enum {
	fmi2_true=fmi2True,
	fmi2_false=fmi2False
} fmi2_boolean_enu_t;

#endif /* End of header file FMI2_TYPES_H_ */
