#include <stdio.h>
#include "main.h"

void fmi1_null_logger(  fmi1_component_t    c,
                        fmi1_string_t   instanceName,
                        fmi1_status_t   status,
                        fmi1_string_t   category,
                        fmi1_string_t   message, ... );

void fmi2_null_logger(  fmi2_component_t c,
                        fmi2_string_t instanceName,
                        fmi2_status_t status,
                        fmi2_string_t category,
                        fmi2_string_t message, ...);

void importlogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message);