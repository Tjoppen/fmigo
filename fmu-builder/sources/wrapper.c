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

//having hand-written getters and setters
#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters

// This check serves two purposes:
// - protect against senseless zero-real FMUs
// - avoid compilation problems on Windows (me_simulation::partials becoming [0])
#if NUMBER_OF_REALS == 0
#error NUMBER_OF_REALS == 0 does not make sense for ModelExchange
#endif

typedef struct {
    fmi2ValueReference unknown, known, vr;
} partial_t;

typedef struct {
    cgsl_simulation sim;

    //we're guaranteed never to need more than (number of reals)^2 partials
    //this could be optimized slightly, using number of real inputs * number of real outputs
    //doing it this way avoids a bunch of allocations
    partial_t partials[NUMBER_OF_REALS*NUMBER_OF_REALS];
    size_t npartials;

    fmi2_import_t *FMU;
    char* dir;
    jm_callbacks m_jmCallbacks;
    Backup m_backup;
} me_simulation;

#define SIMULATION_TYPE    me_simulation
#include "fmuTemplate.h"
#include "hypotmath.h"

#define SIMULATION_INSTANTIATE      wrapper_instantiate
#define SIMULATION_SETUP_EXPERIMENT wrapper_setup_experiment
#define SIMULATION_SET              wrapper_set
#define SIMULATION_GET              wrapper_get
#define SIMULATION_ENTER_INIT       wrapper_enter_init
#define SIMULATION_EXIT_INIT        wrapper_exit_init
#define SIMULATION_FREE             wrapper_free

#include "strlcpy.h"

static fmi2Status generated_fmi2GetReal(ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    return (fmi2Status)fmi2_import_get_real(comp->s.simulation.FMU,vr,nvr,value);
}

static fmi2Status generated_fmi2SetReal(ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    if( comp->s.simulation.FMU != NULL)
        return (fmi2Status)fmi2_import_set_real(comp->s.simulation.FMU,vr,nvr,value);
    return fmi2Error;
}

//fmi2GetInteger and fmi2SetInteger become special because we've introduced integrator
static fmi2Status generated_fmi2GetInteger(ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    size_t x = 0;

    for (x = 0; x < nvr; x++) {
        if (vr[x] == VR_INTEGRATOR) {
            value[x] = md->integrator;
        } else if (fmi2_import_get_integer(comp->s.simulation.FMU, &vr[x], 1, &value[x]) != fmi2OK) {
            return fmi2Error;
        }
    }

    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    if( comp->s.simulation.FMU != NULL) {
        size_t x;

        //only call fmi2SetInteger() on the FMU if it's an integer it expects
        //this fixes FMUs created by FMI Toolbox crashing
        for (x = 0; x < nvr; x++) {
            if (vr[x] == VR_INTEGRATOR) {
                md->integrator = value[x];
            } else if (fmi2_import_set_integer(comp->s.simulation.FMU,&vr[x],1,&value[x]) != fmi2OK) {
                return fmi2Error;
            }
        }

        return fmi2OK;
    }
    return fmi2Error;
}

static fmi2Status generated_fmi2GetBoolean(ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    return (fmi2Status)fmi2_import_get_boolean(comp->s.simulation.FMU,vr,nvr,value);
}

static fmi2Status generated_fmi2SetBoolean(ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    if( comp->s.simulation.FMU != NULL)
        return (fmi2Status)fmi2_import_set_boolean(comp->s.simulation.FMU,vr,nvr,value);
    return fmi2Error;
}

static fmi2Status generated_fmi2GetString(ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    return (fmi2Status)fmi2_import_get_string(comp->s.simulation.FMU,vr,nvr,value);
}

static fmi2Status generated_fmi2SetString(ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    if( comp->s.simulation.FMU != NULL) {
        size_t x;

        for (x = 0; x < nvr; x++) {
            if (vr[x] == VR_OCTAVE_OUTPUT) {
                strlcpy(comp->s.md.octave_output, value[x], sizeof(comp->s.md.octave_output));
            } else if (fmi2_import_set_string(comp->s.simulation.FMU,&vr[x],1,&value[x]) != fmi2OK) {
                return fmi2Error;
            }
        }

        return fmi2OK;
    }
    return fmi2Error;
}

fmi2Status wrapper_get ( me_simulation *sim) {
    storeStates(&sim->sim, &sim->m_backup);
    return fmi2OK;
}

fmi2Status wrapper_set ( me_simulation *sim) {
    restoreStates(&sim->sim, &sim->m_backup);
    return fmi2OK;
}

static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial){
    size_t x;
    state_t *s = &comp->s;

    if ( fmi2_import_get_capability(comp->s.simulation.FMU, fmi2_me_providesDirectionalDerivatives ) ){
      fmi2_value_reference_t vrp []  = {vr};
      fmi2_value_reference_t wrtp[]  = {wrt};
      fmi2_value_reference_t zrefp[] = {0};
      fmi2Real dv [] = {1.0};
      // TODO: move this logic to the fmuTemplate_impl.h
      return (fmi2Status)fmi2_import_get_directional_derivative(
        comp->s.simulation.FMU,
        wrtp, 1,  //v_ref aka known ("Value references for the seed vector")
        vrp, 1,   //z_ref aka unknown ("Value references for the derivatives/outputs to be processed")
        dv,       //dv
        partial   //dz
      );
    }

    //could speed this up with binary search or some kind of simple hash map
    for (x = 0; x < s->simulation.npartials; x++) {
        if (vr  == s->simulation.partials[x].unknown &&
            wrt == s->simulation.partials[x].known) {
            return generated_fmi2GetReal(comp, &s->md, &s->simulation.partials[x].vr, 1, partial);
        }
    }

    //compute d(vr)/d(wrt) = (vr1 - vr0) / (wrt1 - wrt0)
    //since the FMU is ME we don't need to doStep() for the values of vr to update when changing wrt
    fmi2Real dwrt, wrt0, wrt1, vr0, vr1;

    //save state
    wrapper_get(&s->simulation);

    generated_fmi2GetReal(comp, &s->md, &wrt, 1, &wrt0);
    generated_fmi2GetReal(comp, &s->md, &vr,  1, &vr0);

    //take a small step, deal with subnormals
    //2.0^(-1022) = 2.2251e-308
    //if we step by 1 ppm we should get around 10 decimals of precision (doubles are 16 decimals, 16-6 = 10)
    //start kicking in a bit before the subnormal range
    if (fabs(wrt0) < 1.0e-307) {
        //at least 1 ppm of number, or more
        dwrt = wrt0 < 0 ? -1.0e-313 : 1.0e-313; //-307 - 6 = -313
    } else {
        dwrt = 1e-6 * wrt0;
    }

    wrt1 = wrt0 + dwrt;

    generated_fmi2SetReal(comp, &s->md, &wrt, 1, &wrt1);
    generated_fmi2GetReal(comp, &s->md, &vr,  1, &vr1);

    //restore state
    wrapper_set(&s->simulation);

    *partial = (vr1 - vr0) / dwrt;
    return fmi2OK;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    //fprintf(stderr,"do step run iteration\n");
    runIteration(&s->simulation.sim, currentCommunicationPoint,communicationStepSize);
}

//extern "C"{
void jmCallbacksLogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
    fprintf(stderr, "[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

static fmi2Status wrapper_instantiate(ModelInstance *comp)  {
    fprintf(stderr,"Init Wrapper\n");
    state_t *s = &comp->s;
    //char *m_fmuLocation;
    char m_resourcePath[1024];
    char m_fmuPath[1024];
    const char* m_instanceName;
    fmi_version_enu_t m_version;
    fmi_import_context_t* m_context;

    comp->s.simulation.m_jmCallbacks.malloc = malloc;
    comp->s.simulation.m_jmCallbacks.calloc = calloc;
    comp->s.simulation.m_jmCallbacks.realloc = realloc;
    comp->s.simulation.m_jmCallbacks.free = free;
    comp->s.simulation.m_jmCallbacks.logger = jmCallbacksLogger;
    comp->s.simulation.m_jmCallbacks.log_level = 0;
    comp->s.simulation.m_jmCallbacks.context = 0;

    comp->s.simulation.m_jmCallbacks.logger(NULL,"modulename",0,"jm_string");

    strlcpy(m_fmuPath, comp->fmuResourceLocation, sizeof(m_fmuPath));
    strlcat(m_fmuPath, "/", sizeof(m_fmuPath));
    strlcat(m_fmuPath, fmuFilename, sizeof(m_fmuPath));

    if (!(comp->s.simulation.dir = fmi_import_mk_temp_dir(&comp->s.simulation.m_jmCallbacks, NULL, "wrapper_"))) {
        fprintf(stderr, "fmi_import_mk_temp_dir() failed\n");
        exit(1);
    }

    m_context = fmi_import_allocate_context(&comp->s.simulation.m_jmCallbacks);
    fprintf(stderr," cont :%p\n",m_context);
    // unzip the real fmu
    m_version = fmi_import_get_fmi_version(m_context, m_fmuPath, comp->s.simulation.dir);
    fprintf(stderr,"%s\n",m_fmuPath);
    fprintf(stderr,"wrapper: got version %d\n",m_version);

    if ((m_version <= fmi_version_unknown_enu) || (m_version >= fmi_version_unsupported_enu)) {

        fmi_import_free_context(m_context);
        fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
        exit(1);
    }
    if (m_version == fmi_version_2_0_enu) { // FMI 2.0
        // parse the xml file
        /* fprintf(stderr,"dir: %s\n",dir); */
        comp->s.simulation.FMU = fmi2_import_parse_xml(m_context, comp->s.simulation.dir, 0);
        fprintf(stderr,"dir: %s\n",comp->s.simulation.dir);
        if(!comp->s.simulation.FMU) {
        fprintf(stderr,"dir: %s\n",comp->s.simulation.dir);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
            return fmi2Error;
        }

        // check FMU kind
        fmi2_fmu_kind_enu_t fmuType = fmi2_import_get_fmu_kind(comp->s.simulation.FMU);
        if(fmuType != fmi2_fmu_kind_me) {
            fprintf(stderr,"Wrapper only supports model exchange\n");
            fmi2_import_free(comp->s.simulation.FMU);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
            return fmi2Error;
        }
        // FMI callback functions
        const fmi2_callback_functions_t m_fmi2CallbackFunctions = {fmi2_log_forwarding, calloc, free, 0, 0};

        // Load the binary (dll/so)
        jm_status_enu_t status = fmi2_import_create_dllfmu(comp->s.simulation.FMU, fmuType, &m_fmi2CallbackFunctions);
        if (status == jm_status_error) {
            fmi2_import_free(comp->s.simulation.FMU);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
            return fmi2Error;
        }
        m_instanceName = fmi2_import_get_model_name(comp->s.simulation.FMU);
        {
            //m_fmuLocation = fmi_import_create_URL_from_abs_path(&comp->s.simulation.m_jmCallbacks, m_fmuPath);
        }
        fprintf(stderr,"have instancename %s\n",m_instanceName);

        {
            char *temp = fmi_import_create_URL_from_abs_path(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
            strlcpy(m_resourcePath, temp, sizeof(m_resourcePath));
            strlcat(m_resourcePath, "/resources", sizeof(m_resourcePath));
            comp->s.simulation.m_jmCallbacks.free(temp);
        }
    } else {
        // todo add FMI 1.0 later on.
        fmi_import_free_context(m_context);
        fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
        return fmi2Error;
    }
    fmi_import_free_context(m_context);

    fmi2_status_t status = fmi2_import_instantiate(comp->s.simulation.FMU , m_instanceName, fmi2_model_exchange, m_resourcePath, 0);
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: instatiate faild\n");
        return fmi2Error;
    }

    return fmi2OK;
}

static fmi2Status wrapper_setup_experiment(ModelInstance *comp,
        fmi2Boolean toleranceDefined, fmi2Real tolerance,
        fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {
    return (fmi2Status)fmi2_import_setup_experiment(comp->s.simulation.FMU, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime) ;
}

static fmi2Status wrapper_enter_init(ModelInstance *comp) {
    return (fmi2Status)fmi2_import_enter_initialization_mode(comp->s.simulation.FMU) ;
}

static fmi2Status wrapper_exit_init(ModelInstance *comp) {
    FILE *fp;
    fmi2Status status;

    //PATH_MAX is usually 512 or so, this should be enough
    char path[1024];

    prepare(&comp->s.simulation.sim, comp->s.simulation.FMU, comp->s.md.integrator);
    status = (fmi2Status)fmi2_import_exit_initialization_mode(comp->s.simulation.FMU);
    if(status == fmi2Error){
        return status;
    }
    status = (fmi2Status)fmi2_import_enter_continuous_time_mode(comp->s.simulation.FMU);
    if(status == fmi2Error){
        return status;
    }

    allocateBackup(&comp->s.simulation.m_backup, comp->s.simulation.sim.model->parameters);

    strlcpy(path, comp->fmuResourceLocation, sizeof(path));
    strlcat(path, "/directional.txt",        sizeof(path));

    fp = fopen(path, "r");

    if (fp) {
        //read partials from directional.txt
        me_simulation *me = &comp->s.simulation;

        for (;;) {
            partial_t *p = &me->partials[me->npartials];
            int n = fscanf(fp, "%u %u %u", &p->unknown, &p->known, &p->vr);

            if (n < 3) {
                break;
            }

            if (++me->npartials >= sizeof(me->partials)/sizeof(me->partials[0])) {
                break;
            }
        }

        fclose(fp);
    }

    return fmi2OK;
}



static void wrapper_free(me_simulation me) {
  fmi2_import_terminate(me.FMU);
  fmi2_import_free_instance(me.FMU);
  fmi2_import_destroy_dllfmu(me.FMU);
  fmi2_import_free(me.FMU);
  fmi_import_rmdir(&me.m_jmCallbacks, me.dir);
  free(me.dir);
  freeBackup(&me.m_backup);
  cgsl_free_simulation(me.sim);
}

#include "fmuTemplate_impl.h"
