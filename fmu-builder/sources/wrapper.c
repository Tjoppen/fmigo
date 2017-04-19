#ifndef WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif

//link fmilib statically
#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>

#include <stdio.h>
#include <stdlib.h>

//#include "modelDescription_me.h"
#include "modelExchange.h"
#include "modelDescription.h"
#define SIMULATION_TYPE    cgsl_simulation*
#include "fmuTemplate.h"
#include "hypotmath.h"

#define SIMULATION_WRAPPER wrapper_ntoeu
#define SIMULATION_SET     wrapper_set
#define SIMULATION_GET     wrapper_get
#define MODEL_INIT         model_init   //like SIMULATION_INIT but get a ModelInstance* instead of state_t*

typedef struct vr{
    fmi2ValueReference ws;
    fmi2ValueReference ww;
    fmi2ValueReference ts;
    fmi2ValueReference tw;
    fmi2ValueReference a11;
    fmi2ValueReference a12;
    fmi2ValueReference a22;
}vr;
static vr vrs;

#include "strlcpy.h"

void model_init(ModelInstance *comp) {
    FILE *fp;

    //PATH_MAX is usually 512 or so, this should be enough
    char path[1024];

    strlcpy(path, comp->fmuResourceLocation, sizeof(path));
    strlcat(path, "/directional.txt",        sizeof(path));

    fp = fopen(path, "r");

    if(!fp){
        fprintf(stderr,"Wrapper: no such file or directory -- %s\n", path);
        return;
    }
    int r;
    if ((r = fscanf(fp, "%u %u %u %u %u %u %u", &vrs.ws,&vrs.ww,&vrs.ts,&vrs.tw,&vrs.a11,&vrs.a12,&vrs.a22)) != 7){
        fprintf(stderr,"have read %d valuereferences, expect 7\n",r);
        exit(1);
    }
    fclose(fp);
}


fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt,fmi2Real *partial){
    if (vr == vrs.ws) {
        if (wrt == vrs.ts)
            return generated_fmi2GetReal(&s->md,&vrs.a11,1,partial);
        if (wrt == vrs.tw)
            return generated_fmi2GetReal(&s->md,&vrs.a12,1,partial);
    }
    if (vr == vrs.ww) {
        if (wrt == vrs.ts)
            return generated_fmi2GetReal(&s->md,&vrs.a12,1,partial);
        if (wrt == vrs.tw)
            return generated_fmi2GetReal(&s->md,&vrs.a22,1,partial);
    }
    return fmi2Error;
}

fmi2Status SIMULATION_GET ( SIMULATION_TYPE *sim) {
    fprintf(stderr,"GET: %p\n", sim);
    storeStates(*sim, getTempBackup());
    fprintf(stderr,"GET: %p done \n", sim);
    return fmi2OK;
}

fmi2Status SIMULATION_SET ( SIMULATION_TYPE *sim) {
    fprintf(stderr,"SET: %p\n", sim);
    restoreStates(*sim, getTempBackup());
    storeStates(*sim, getBackup());
    fprintf(stderr,"SET: %p done\n", sim);
    return fmi2OK;
}

void SIMULATION_WRAPPER(ModelInstance *comp);
#define NEW_DOSTEP //to get noSetFMUStatePriorToCurrentPoint
static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    fprintf(stderr,"do step run iteration\n");
 runIteration(currentCommunicationPoint,communicationStepSize, getBackup());
}


//gcc -g clutch.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){
    return 0;
}
#else

#include "fmuTemplate_impl.h"

#endif

//extern "C"{
void jmCallbacksLogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
    fprintf(stderr, "[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

void SIMULATION_WRAPPER(ModelInstance *comp)  {
    fprintf(stderr,"Init Wrapper\n");
    state_t *s = &comp->s;
    //char *m_fmuLocation;
    char m_resourcePath[1024];
    char m_fmuPath[1024];
    char* dir;
    const char* m_instanceName;
    fmi2_import_t** FMU = getFMU();
    jm_callbacks m_jmCallbacks;
    fmi_version_enu_t m_version;
    fmi_import_context_t* m_context;

    m_jmCallbacks.malloc = malloc;
    m_jmCallbacks.calloc = calloc;
    m_jmCallbacks.realloc = realloc;
    m_jmCallbacks.free = free;
    m_jmCallbacks.logger = jmCallbacksLogger;
    m_jmCallbacks.log_level = 0;
    m_jmCallbacks.context = 0;

    m_jmCallbacks.logger(NULL,"modulename",0,"jm_string");

    strlcpy(m_fmuPath, comp->fmuResourceLocation, sizeof(m_fmuPath));
    strlcat(m_fmuPath, "/", sizeof(m_fmuPath));
    strlcat(m_fmuPath, s->md.fmu, sizeof(m_fmuPath));

    if (!(dir = fmi_import_mk_temp_dir(&m_jmCallbacks, NULL, "wrapper_"))) {
        fprintf(stderr, "fmi_import_mk_temp_dir() failed\n");
        exit(1);
    }

    m_context = fmi_import_allocate_context(&m_jmCallbacks);
    fprintf(stderr," cont :%p\n",m_context);
    // unzip the real fmu
    m_version = fmi_import_get_fmi_version(m_context, m_fmuPath, dir);
    fprintf(stderr,"%s\n",m_fmuPath);
    fprintf(stderr,"wrapper: got version %d\n",m_version);

    if ((m_version <= fmi_version_unknown_enu) || (m_version >= fmi_version_unsupported_enu)) {

        fmi_import_free_context(m_context);
        fmi_import_rmdir(&m_jmCallbacks, dir);
        exit(1);
    }
    if (m_version == fmi_version_2_0_enu) { // FMI 2.0
        // parse the xml file
        /* fprintf(stderr,"dir: %s\n",dir); */
        *FMU = fmi2_import_parse_xml(m_context, dir, 0);
        fprintf(stderr,"dir: %s\n",dir);
        if(!*FMU) {
        fprintf(stderr,"dir: %s\n",dir);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&m_jmCallbacks, dir);
            return;
        }

        // check FMU kind
        fmi2_fmu_kind_enu_t fmuType = fmi2_import_get_fmu_kind(*FMU);
        if(fmuType != fmi2_fmu_kind_me) {
            fprintf(stderr,"Wrapper only supports model exchange\n");
            fmi2_import_free(*FMU);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&m_jmCallbacks, dir);
            return;
        }
        // FMI callback functions
        const fmi2_callback_functions_t m_fmi2CallbackFunctions = {fmi2_log_forwarding, calloc, free, 0, 0};

        // Load the binary (dll/so)
        jm_status_enu_t status = fmi2_import_create_dllfmu(*FMU, fmuType, &m_fmi2CallbackFunctions);
        if (status == jm_status_error) {
            fmi2_import_free(*FMU);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&m_jmCallbacks, dir);
            return;
        }
        m_instanceName = fmi2_import_get_model_name(*FMU);
        {
            //m_fmuLocation = fmi_import_create_URL_from_abs_path(&m_jmCallbacks, m_fmuPath);
        }
        fprintf(stderr,"have instancename %s\n",m_instanceName);

        {
            char *temp = fmi_import_create_URL_from_abs_path(&m_jmCallbacks, dir);
            strlcpy(m_resourcePath, temp, sizeof(m_resourcePath));
            strlcat(m_resourcePath, "/resources", sizeof(m_resourcePath));
            m_jmCallbacks.free(temp);
        }
#ifndef WIN32
        //prepare HDF5
        //getHDF5Info();
#endif
    } else {
        // todo add FMI 1.0 later on.
        fmi_import_free_context(m_context);
        fmi_import_rmdir(&m_jmCallbacks, dir);
        return;
    }
    free(dir);

    fmi2_status_t status = fmi2_import_instantiate(*FMU , m_instanceName, fmi2_model_exchange, m_resourcePath, 0);
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: instatiate faild\n");
        exit(1);
    }

    //setup_experiment
    status = fmi2_import_setup_experiment(*FMU, true, 1e-6, 0, false, 0) ;
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: setup Experiment faild\n");
        exit(1);
    }

    status = fmi2_import_enter_initialization_mode(*FMU) ;
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: enter initialization mode faild\n");
        exit(1);
    }
    prepare();
    s->simulation = getSim();
    status = fmi2_import_exit_initialization_mode(*FMU);
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: exit initialization mode faild\n");
        exit(1);
    }
}
