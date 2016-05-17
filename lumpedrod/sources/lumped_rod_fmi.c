#include "lumped_rod.h"
#include <math.h>



#define MODEL_IDENTIFIER lumpedrod
#define MODEL_GUID "{b8998512-96a7-4e6d-8350-6d1f9aeae4a1}"

enum {
/* all outputs */
  THETA1,      //angle (output, state)
  THETA2,      //angle (output, state)
  OMEGA1,      //angular velocity (output, state)
  OMEGA2,      //angular velocity (output, state)
  ALPHA1,      //angular acceleration (output)
  ALPHA2,      //angular acceleration (output)
  DTHETA1,     /* estimated angle difference */
  DTHETA2,     /* estimated angle difference */
  TAU_OUT1,    /* internal torque computed on first element */
  TAU_OUT2,    /* internal torque computed on last element */
/* inputs */
  TAU1,				// driving torque
  TAU2,				// driving torque
  OMEGA_DRIVE1,			/* driving velocity */
  OMEGA_DRIVE2,			/* driving velocity */

  J0,          // moment of inertia [1/(kg*m^2)] (parameter)
  STIFFNESS,   // stiffness of the rod
  RELAX,       //relaxation parameter
  STIFFNESS1,  // stiffness of first velocity driver
  RELAX1,      //relaxation parameter of driver
  STIFFNESS2,  // stiffness of last velocity driver
  RELAX2,      //relaxation parameter of driver
  STEP,        // *internal* time step
  NUMBER_OF_REALS
};

enum {
  NELEM,                            // number of elements
  NUMBER_OF_INTEGERS
};


#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0
#define FMI_COSIMULATION

#define SIMULATION_TYPE lumped_rod_sim
//called after getting default values from XML
#define SIMULATION_INIT setStartValues
#define SIMULATION_FREE lumped_rod_sim_free
#define SIMULATION_GET lumped_rod_sim_store
#define SIMULATION_SET lumped_rod_sim_restore

#include "fmuTemplate.h"


static void lumped_rod_fmi_sync_out( lumped_rod_sim * sim, state_t *s){
  
  s->r[ THETA1   ]  = sim->state.state.x1;
  s->r[ THETA2   ]  = sim->state.state.xN;
  s->r[ OMEGA1   ]  = sim->state.state.v1;
  s->r[ OMEGA2   ]  = sim->state.state.vN;
  s->r[ ALPHA1   ]  = sim->state.state.a1;
  s->r[ ALPHA2   ]  = sim->state.state.aN;

  s->r[ ALPHA1   ]  = sim->state.state.a1;
  s->r[ ALPHA2   ]  = sim->state.state.aN;
  
  s->r[ DTHETA1  ]  = sim->state.state.dx1;
  s->r[ DTHETA2  ]  = sim->state.state.dxN;

  s->r[ TAU_OUT1 ]  = sim->state.state.f1;
  s->r[ TAU_OUT2 ]  = sim->state.state.fN;

  return;
  
}

static void lumped_rod_fmi_sync_in( lumped_rod_sim * sim, state_t *s){
  
  sim->state.state.driver_f1  =  s->r[ TAU1 ];
  sim->state.state.driver_fN  =  s->r[ TAU2 ];
  sim->state.state.driver_v1  =  s->r[ OMEGA_DRIVE1 ];
  sim->state.state.driver_vN  =  s->r[ OMEGA_DRIVE2 ];

  return;

}
 
/**
   Instantiate the simulation and set initial conditions.
*/
static void setStartValues(state_t *s) {
  /** read the init values given by the master, either from command line
      arguments or as defaults from modelDescription.xml
  */
  int i; 
  lumped_rod_sim_parameters p = { 
    s->r[ STEP ],
    {
      /** these are normally outputs */
      s->r[ THETA1       ],
      s->r[ THETA2       ],
      s->r[ OMEGA1       ],
      s->r[ OMEGA2       ],
      s->r[ ALPHA1       ],
      s->r[ ALPHA2       ],
      s->r[ DTHETA1      ],
      s->r[ DTHETA2      ],
      s->r[ TAU_OUT1     ],
      s->r[ TAU_OUT2     ],
      /** these are outputs */
      s->r[ TAU1         ],
      s->r[ TAU2         ],
      s->r[ OMEGA_DRIVE1 ],
      s->r[ OMEGA_DRIVE2 ]
    },
    {
      s->i[ NELEM ], /** physical parameters*/
      s->r[ J0 ], 
      s->r[ STIFFNESS ],
      s->r[ RELAX ],
      
      s->r[ STIFFNESS1 ],
      s->r[ RELAX1 ],
      
      s->r[ STIFFNESS2 ],
      s->r[ RELAX2 ]
    }
  };

  s->simulation = lumped_rod_sim_create( p ); 
  FILE * f = fopen("/tmp/z", "w+");
  fprintf(f, "Init conditions.   Step  = %f \n", s->r[ STEP ]);
  fprintf(f, "[ ... \n");
  for ( i = 0; i < s->i[ NELEM ]; ++i ){
    fprintf(stderr, "%f; ... \n", s->simulation.rod.state.x[ i ] );
  }
    
  fclose(f);

};

/** Returns partial derivative of vr with respect to wrt  
 *  We could define a smart convention here.  
 */ 
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
  if (vr == ALPHA1 && wrt == TAU1 ) {
    *partial = s->simulation.rod.mobility[ 0 ];
    return fmi2OK;
  }

  if (vr == ALPHA1 && wrt == TAU2 ) {
    *partial = s->simulation.rod.mobility[ 1 ];
    return fmi2OK;
  }

  if (vr == ALPHA2 && wrt == TAU1 ) {
    *partial = s->simulation.rod.mobility[ 2 ];
    return fmi2OK;
  }
    
  if (vr == ALPHA2 && wrt == TAU2 ) {
    *partial = s->simulation.rod.mobility[ 3 ];
    return fmi2OK;
  }

  return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
  /*  Copy the input variable from the state vector */
  lumped_rod_fmi_sync_in(&s->simulation, s);

  int n = ( int ) ceil( communicationStepSize / s->simulation.state.step );
  /* Execute the simulation */
  rod_sim_do_step(&s->simulation , n );
  /* Copy state variables to ouputs */
  lumped_rod_fmi_sync_out(&s->simulation, s);
  
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

