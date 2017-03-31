//TODO: add some kind of flag that switches this one between a clutch and a gearbox, to reduce the amount of code needed
#define SIMULATION_INIT wrapper_init
#include <fmilib.h>
#include "modelDescription.h"
#include "modelExchangeFmiInterface.h"
#include "modelExchange.h"
#include "fmuTemplate.h"

#include "/tmp/printcwd.h"
#include <unistd.h>
void wrapper_init(state_t *s) ;
#define NEW_DOSTEP //to get noSetFMUStatePriorToCurrentPoint
static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    printcwd();
//runIteration(currentCommunicationPoint,communicationStepSize);
}


//gcc -g clutch.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){
  return 0;
}
#else

#include "fmuTemplate_impl.h"


#endif
#include <dirent.h>
//extern "C"{
void jmCallbacksLogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
  fprintf(stderr, "[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

/* void wrapper_init(state_t *s)  { */
/*     fprintf(stderr,"Init Wrapper\n"); */
/*     char *m_fmuLocation; */
/*     char *m_resourcePath; */
/*     char* m_fmuPath; */
/*     char* dir; */
/*     const char* m_instanceName; */
/*   wrapper.m_jmCallbacks.malloc = malloc; */
/*   wrapper.m_jmCallbacks.calloc = calloc; */
/*   wrapper.m_jmCallbacks.realloc = realloc; */
/*   wrapper.m_jmCallbacks.free = free; */
/*   wrapper.m_jmCallbacks.logger = jmCallbacksLogger; */
/*   wrapper.m_jmCallbacks.log_level = 0; */
/*   wrapper.m_jmCallbacks.context = 0; */
/*   m_fmuPath = (char*)calloc(1024,sizeof(char)); */

/*   char cwd[1024]; */
/*   if (getcwd(cwd, sizeof(cwd)) == NULL) */
/*     exit(1); */

/*   //strcpy(m_fmuPath, dir); */
/*   strcpy(m_fmuPath,cwd); */
/*   strcat(m_fmuPath, "/"); */
/*   strcat(m_fmuPath, s->md.fmu); */

/*   if (!(dir = fmi_import_mk_temp_dir(&wrapper.m_jmCallbacks, NULL, "wrapper_"))) { */
/*     fprintf(stderr, "fmi_import_mk_temp_dir() failed\n"); */
/*     exit(1); */
/*   } */

/*   wrapper.m_context = fmi_import_allocate_context(&wrapper.m_jmCallbacks); */
/*   // unzip the real fmu */
/*   wrapper.m_version = fmi_import_get_fmi_version(wrapper.m_context, m_fmuPath, dir); */

/*   if ((wrapper.m_version <= fmi_version_unknown_enu) || (wrapper.m_version >= fmi_version_unsupported_enu)) { */

/*     fmi_import_free_context(wrapper.m_context); */
/*     fmi_import_rmdir(&wrapper.m_jmCallbacks, dir); */
/*     exit(1); */
/*   } */
/*   if (wrapper.m_version == fmi_version_2_0_enu) { // FMI 2.0 */
/*     // parse the xml file */
/*     wrapper.m_fmi2Instance = fmi2_import_parse_xml(wrapper.m_context, dir, 0); */
/*     if(!wrapper.m_fmi2Instance) { */
/*       fmi_import_free_context(wrapper.m_context); */
/*       fmi_import_rmdir(&wrapper.m_jmCallbacks, dir); */
/*       return; */
/*     } */

/*     // check FMU kind */
/*     fmi2_fmu_kind_enu_t fmuType = fmi2_import_get_fmu_kind(wrapper.m_fmi2Instance); */
/*     if(fmuType != fmi2_fmu_kind_me) { */
/*       fprintf(stderr,"Wrapper only supports model exchange\n"); */
/*       fmi2_import_free(wrapper.m_fmi2Instance); */
/*       fmi_import_free_context(wrapper.m_context); */
/*       fmi_import_rmdir(&wrapper.m_jmCallbacks, dir); */
/*       return; */
/*     } */
/*     // FMI callback functions */
/*     const fmi2_callback_functions_t m_fmi2CallbackFunctions = {fmi2_log_forwarding, calloc, free, 0, 0}; */

/*     // Load the binary (dll/so) */
/*     jm_status_enu_t status = fmi2_import_create_dllfmu(wrapper.m_fmi2Instance, fmuType, &m_fmi2CallbackFunctions); */
/*     if (status == jm_status_error) { */
/*       fmi2_import_free(wrapper.m_fmi2Instance); */
/*       fmi_import_free_context(wrapper.m_context); */
/*       fmi_import_rmdir(&wrapper.m_jmCallbacks, dir); */
/*       return; */
/*     } */
/*      m_instanceName = fmi2_import_get_model_name(wrapper.m_fmi2Instance); */
/*     { */
/*         //m_fmuLocation = fmi_import_create_URL_from_abs_path(&wrapper.m_jmCallbacks, m_fmuPath); */
/*     } */

/*     { */
/*         char *temp = fmi_import_create_URL_from_abs_path(&wrapper.m_jmCallbacks, dir); */
/*         m_resourcePath = (char*)calloc(strlen(temp)+11,sizeof(char)); */
/*         strcpy(m_resourcePath,temp); */
/*         strcat(m_resourcePath,"/resources"); */
/*         wrapper.m_jmCallbacks.free(temp); */
/*     }    /\* 0 - original order as found in the XML file; */
/*      * 1 - sorted alphabetically by variable name; */
/*      * 2 sorted by types/value references. */
/*      *\/ */
/*     int sortOrder = 0; */
/*     wrapper.m_fmi2Variables = fmi2_import_get_variable_list(wrapper.m_fmi2Instance, sortOrder); */
/*     wrapper.m_fmi2Outputs = fmi2_import_get_outputs_list(wrapper.m_fmi2Instance); */

/* #ifndef WIN32 */
/*     //prepare HDF5 */
/*     //getHDF5Info(); */
/* #endif */
/*   } else { */
/*     // todo add FMI 1.0 later on. */
/*     fmi_import_free_context(wrapper.m_context); */
/*     fmi_import_rmdir(&wrapper.m_jmCallbacks, dir); */
/*     return; */
/*   } */
/*   free(dir); */
/*   free(m_resourcePath); */
/*   free(m_fmuPath); */
/*   wrapper.m_fmi2Instance = fmi2Instantiate(m_instanceName, fmi2_fmu_kind_me, MODEL_GUID, "", &wrapper.m_fmi2CallbackFunctions, false, false); */
/*   //setup_experiment */
/*   fmi2Status status = fmi2SetupExperiment(wrapper.m_fmi2Instance, true, 1e-6, 0, false, 0) ; */
/*   if(status == fmi2Error){ */
/*       fprintf(stderr,"Wrapper: setup Experiment faild\n"); */
/*       exit(1); */
/*   } */
/*   status = fmi2EnterInitializationMode(wrapper.m_fmi2Instance) ; */
/*   if(status == fmi2Error){ */
/*       fprintf(stderr,"Wrapper: enter initialization mode faild\n"); */
/*       exit(1); */
/*   } */
/*   prepare(); */
/*   status = fmi2ExitInitializationMode(wrapper.m_fmi2Instance); */
/* } */
