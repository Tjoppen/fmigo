//TODO: add some kind of flag that switches this one between a clutch and a gearbox, to reduce the amount of code needed
//#define SIMULATION_INIT wrapper_ntoeu
#ifndef WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif
#include <fmilib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#include "modelDescription_me.h"
#include "commonWrapper/modelExchange.h"
#include "modelDescription.h"
#define SIMULATION_TYPE    cgsl_simulation*
#include "fmuTemplate.h"
#include "hypotmath.h"

#define SIMULATION_WRAPPER wrapper_ntoeu
#define SIMULATION_INIT    wrapper_init
#define SIMULATION_SET     wrapper_set
#define SIMULATION_GET     wrapper_get

typedef struct vr{
    int vr1;
    int vr2;
    int vr3;
    int valid;
}vr;
vr vrs;
void setVR(vr* vrs, int i, int value){
    switch (i){
    case 0: vrs->vr1 = value; break;
    case 1: vrs->vr2 = value; break;
    case 2: vrs->vr3 = value; break;
    default: {
        fprintf(stderr,"Wrapper: setVR -- index %i is out of range\n", i);
        exit(1);
    }
    }
}

vr initVR(){
    vr vrs = {0,0,0,0};
    return vrs;
}

int isValitVR(vr *vrs){
    return vrs->valid;
}

vr stringToVr(char *string){
    vr vrs = initVR();
    if(strlen(string) == 0 )
        return vrs;

    char *tmp = string;
    char* size = string + strlen(string);

    int i = 0;
    int index = 0;
    for(;string != size; string++){
        if(*string != ','){
            tmp[i++] = *string ;
        }
        else{
            tmp[i] = '\0';
            i = 0;
            setVR(&vrs, index++, atoi(tmp));
            tmp = string;
        }
    }
    //tmp[i] = '\0';
    setVR(&vrs, index++, atoi(tmp));
    vrs.valid = 1;
    return vrs;
}
vr extractVR(char *string){
    int i = 0;
    char *tmp = string;
    char* size = string + strlen(string);
    for(;string != size; string++){
        if(*string != ':'){
            tmp[i++] = *string ;
        }
        else{
            tmp[i] = '\0';
            i = 0;
            return stringToVr(tmp);
        }
    }
//tmp[i] = '\0';
    return stringToVr(tmp);
}

static vr directionalVR1;
static vr directionalVR2;
static vr directionalVR3;
static vr directionalVR4;


fmi2Status getPartial(state_t *s, fmi2ValueReference x, fmi2ValueReference unKnown_ref,fmi2Real *partial){
    fmi2Status status;
    fprintf(stderr,"get_partial\n");
    fprintf(stderr,"directional %s\n",s->md.directional);
    *partial = 1;
    if(!isValitVR(&vrs)){
        fprintf(stderr,"Wrapper: Request getPartial but parameter directional not set\n");
        exit(22);
    }


    return status;
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



void SIMULATION_INIT(state_t *s){
    fprintf(stderr,"INIT: has stnig %s \n", s->md.directional);
    directionalVR1 = stringToVr(s->md.directional);
    directionalVR2 = stringToVr(s->md.directional);
    directionalVR3 = stringToVr(s->md.directional);
    directionalVR4 = stringToVr(s->md.directional);
}

void SIMULATION_WRAPPER(state_t *s);
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

void SIMULATION_WRAPPER(state_t *s)  {
    fprintf(stderr,"Init Wrapper\n");
    char *m_fmuLocation;
    char *m_resourcePath;
    char* m_fmuPath;
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
    m_fmuPath = (char*)calloc(1024,sizeof(char));

    m_jmCallbacks.logger(NULL,"modulename",0,"jm_string");
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        exit(1);

    strcpy(m_fmuPath,cwd);
    strcat(m_fmuPath, "/");
    strcat(m_fmuPath, s->md.fmu);

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
            m_resourcePath = (char*)calloc(strlen(temp)+11,sizeof(char));
            strcpy(m_resourcePath,temp);
            strcat(m_resourcePath,"/resources");
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
    free(m_resourcePath);
    free(m_fmuPath);

    fmi2Status status = fmi2_import_instantiate(*FMU , m_instanceName, fmi2_fmu_kind_me, m_resourcePath, 0);
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
