#include "common/gsl_interface.h"
#include <string.h>
#include <math.h>

#ifdef WIN32
//http://stackoverflow.com/questions/6809275/unresolved-external-symbol-hypot-when-using-static-library#10051898
double hypot(double x, double y) {return _hypot(x, y);}
#endif


/**
 *  Convinently package all integrator objects in one place. 
 */
const gsl_odeiv2_step_type * cgsl_get_integrator( int  i ) {

  const gsl_odeiv2_step_type * integrators [] =
    {
      gsl_odeiv2_step_rk2,	/* 0 */
      gsl_odeiv2_step_rk4,	/* 1 */ 
      gsl_odeiv2_step_rkf45,	/* 2 */
      gsl_odeiv2_step_rkck,	/* 3 */
      gsl_odeiv2_step_rk8pd,	/* 4 */
      gsl_odeiv2_step_rk1imp,	/* 5 */
      gsl_odeiv2_step_rk2imp,	/* 6 */
      gsl_odeiv2_step_rk4imp,	/* 7 */
      gsl_odeiv2_step_bsimp,	/* 8 */
      gsl_odeiv2_step_msadams,	/* 9 */
      gsl_odeiv2_step_msbdf	/*10 */
    };

  return integrators [ i ];

}


/**
 * Integrate the equations by one full step.
 * Used by cgsl_step_to() 
 *
 */
static int cgsl_step ( void * _s  ) {

  cgsl_simulation * s = ( cgsl_simulation * ) _s ;
  int status;
  if ( s->fixed_step ) {
    status = gsl_odeiv2_evolve_apply_fixed_step(s->i.evolution, s->i.control, s->i.step, &s->i.system, &s->t, s->h, s->model->x);
  } else {
    status = gsl_odeiv2_evolve_apply(s->i.evolution,
				     s->i.control, s->i.step, &s->i.system, &s->t, s->t_end, &s->h, s->model->x);
  }

  return status;

}

/**
 * Integrate the equations up to a given end point.
 */
int cgsl_step_to(void * _s,  double t_start, double t_end ) {

  cgsl_simulation * s = ( cgsl_simulation * ) _s;

  int i; 

  s->t  = t_start;		/* start point */
  s->t_end = t_end;		/* end point */
  s->steps = 0;
  
  ///
  /// GSL integrators return after each successful step hence the while
  /// loop.
  ///
  while ( s->t < s->t_end)
  {
    int status =  cgsl_step( s );
    s->steps++;

    // integreate  all variables
    
    /// Diagnostics and printing below this point
    if (status != GSL_SUCCESS ){
      fprintf(stderr, "GSL integrator: bad status: %d \n", status);
      exit(-1);
    }

    if ( s->print  && s->file ) { 
      fprintf (s->file, "%.5e ", s->t );
      for ( i = 0; i < s->i.system.dimension; ++i ){
	fprintf (s->file, "%.5e ", s->model->x[ i ]); 
      }
      fprintf (s->file, "\n");
    }
    
  }

  return 0;

}

/**
 * Note: we use pass-by-value semantics for all structs anywhere possible.
 *
 * After the user defines a parameter struct, a function to evaluate the
 * derivatives, and optionally, a function to evaluate the Jacobian and a
 * `free' function, everything GSL related is instantiated here.
 */
cgsl_simulation cgsl_init_simulation(
  cgsl_model * model, /** the model we work on: user defined.  Contains parameters and function pointers. */
  enum cgsl_integrator_ids integrator, /** integrator of choice: set by user  */
  int print,
  FILE *f,
  cgsl_step_control_parameters step_control_parameters
  )
{

  cgsl_simulation sim;

  sim.model                  = model;
  sim.i.step_type            =  cgsl_get_integrator( integrator );

  /** pass model definition to gsl integration structure */
  sim.i.system.function      = model->function;
  sim.i.system.jacobian      = model->jacobian;
  sim.i.system.dimension     = model->n_variables;
  sim.i.system.params        = ( void * ) model->parameters;

  sim.i.evolution            = gsl_odeiv2_evolve_alloc ( model->n_variables );
  sim.i.step                 = gsl_odeiv2_step_alloc (sim.i.step_type, model->n_variables);
  /** sets driver and timestep control */
  cgsl_get_step_control( step_control_parameters, &sim );
  sim.n                      = 0;
  sim.t                      = 0.0;
  sim.t_end                  = 0.0; 
  sim.data                   = ( double * ) NULL;
  sim.buffer_size            = 0;
  sim.file                   = f;
  sim.print                  = print;

  return sim;

}

void cgsl_model_default_free(cgsl_model *model) {
    free(model->x);
    free(model);
}

  
void cgsl_enable_write( cgsl_simulation * sim, int x ) {
  sim->print = x;
  return;
}

cgsl_model* cgsl_model_default_alloc(int n_variables, const double *x0, void *parameters,
				     ode_function_ptr function,
				     ode_jacobian_ptr jacobian, size_t sz) {

    //allocate at least the size of cgsl_model
    if ( sz < sizeof(cgsl_model) ) {
      sz = sizeof(cgsl_model);
    }

    cgsl_model * m = calloc( 1, sz );

    m->n_variables = n_variables;
    m->x           = calloc( n_variables, sizeof(double));

    if (x0) {
        memcpy(m->x, x0, n_variables * sizeof(double));
    }

    m->parameters  = parameters;
    m->function    = function;
    m->jacobian    = jacobian;
    m->free        = cgsl_model_default_free;

    return m;

}

void  cgsl_free_simulation( cgsl_simulation sim ) {

  gsl_odeiv2_driver_free  (sim.i.driver);
  gsl_odeiv2_step_free    (sim.i.step);
  gsl_odeiv2_evolve_free  (sim.i.evolution);
  gsl_odeiv2_control_free (sim.i.control);

  if (sim.data) {
    free(sim.data);
  }

  if (sim.file) {
    fclose(sim.file);
  }

  if (sim.model->free) {
    sim.model->free(sim.model);
  }

  return;
  
}

void cgsl_simulation_set_fixed_step( cgsl_simulation * s, double h){

  s->h = h;
  s->fixed_step = 1; 

  return;
}

void cgsl_simulation_set_variable_step( cgsl_simulation * s ) {

  s->fixed_step = 0; 

  return;
}



/** This will set tolerances to 1e-6 and choose the simpler step control routine.*/

static cgsl_step_control_parameters cgsl_step_control_set_defaults(){
  cgsl_step_control_parameters x = {
    1e-6,			/* relative tolerance */
    1e-6, 			/* absolute tolerance */
    step_control_y_new,		/* standard stepsize control */
    1e-6, 			/* initial step */
    0,				/* scaling of variables */
    0,				/* scaling of derivatives */
    (double *)NULL,		/* individual scalings */
    0				/* number of variables */
  };
  return x;
}



/**
 * Check to see if a step control parameter struct has reasonable values.
 * If not, make default.
 * Returns 0 for a bad value and  1 for OK.
 */

static int cgsl_verify_control_parameters( cgsl_step_control_parameters * p ){

  int ret = 1;
  if ( 
  ! isnormal( p->eps_rel ) ||
  ! isnormal( p->eps_abs ) ||
  ! isnormal( p->start  )  ||
   p->id < step_control_first  || p->id >= invalid_step_control_id
    ){
    ret = 0;
  }
  if ( p->id >= step_control_yp_new && (  ! isnormal( p->a_y) || !isnormal( p->a_dydt ) ) ){
    ret = 0;
  }
  if ( p->id == step_control_scaled_new && p->scale_abs == (double * ) NULL  ){
    ret = 0;
  }
  
  if ( ret == 0 ){
    *p = cgsl_step_control_set_defaults();
  }
  
  return ret;
}



void cgsl_get_step_control( cgsl_step_control_parameters p, cgsl_simulation * sim){


  cgsl_verify_control_parameters(  &p );

  sim->h = p.start;
  sim->fixed_step = 0;
  
  switch( (int) p.id) {
    case   step_control_y_new:
      sim->i.control = gsl_odeiv2_control_y_new (p.eps_abs, p.eps_rel);
      sim->i.driver  = gsl_odeiv2_driver_alloc_y_new (&sim->i.system, sim->i.step_type, p.start, p.eps_abs, p.eps_rel);
      break;
    case   step_control_fixed:
      sim->i.control = gsl_odeiv2_control_y_new (p.eps_abs, p.eps_rel);
      sim->i.driver  = gsl_odeiv2_driver_alloc_y_new (&sim->i.system, sim->i.step_type, p.start, p.eps_abs, p.eps_rel);
      sim->fixed_step = 1;
      break;
    case   step_control_yp_new:
      sim->i.control = gsl_odeiv2_control_yp_new (p.eps_abs, p.eps_rel);
      sim->i.driver  = gsl_odeiv2_driver_alloc_yp_new (&sim->i.system, sim->i.step_type, p.start, p.eps_abs, p.eps_rel);
      break;
    case   step_control_standard_new:
      sim->i.control = gsl_odeiv2_control_yp_new (p.eps_abs, p.eps_rel);
      sim->i.driver  = gsl_odeiv2_driver_alloc_yp_new (&sim->i.system, sim->i.step_type, p.start, p.eps_abs, p.eps_rel);
      break;
  case step_control_scaled_new:
    sim->i.control = gsl_odeiv2_control_standard_new (p.eps_abs, p.eps_rel, p.a_y, p.a_dydt );
    sim->i.driver  = gsl_odeiv2_driver_alloc_standard_new (&sim->i.system, sim->i.step_type, p.start, p.eps_abs, p.eps_rel, p.a_y, p.a_dydt);
    break;
      break;
  }

  gsl_odeiv2_step_set_driver (sim->i.step, sim->i.driver);
  return ;
}


/**
   According to the GSL man page
   https://www.gnu.org/software/gsl/manual/html_node/Stepping-Functions.html#Stepping-Functions
 */

cgsl_integrator_named_ids cgsl_integrator_list[]={
  {rk2,	 "Runge Kutta order 2", "rk2"}, 
  {rk4,	 "Runge Kutta order 4", "rk4"}, 
  {rkf45,"Runge Kutta order 4 with Feldberg pair", "rkf45"}, 
  {rkck,  "Explicit embedded Runge-Kutta Cash-Karp (4, 5) method.", "rkck"}, 
  {rk8pd, "Runge Kutta order 8, Dormand Prince pair", "rk8pd"} , 
  {rk1imp, "Implicit Euler first order", "rk1imp"}, 
  {rk2imp, "Runge Kutta implicit order 2, Gauss's method", "rk2imp"}, 
  {rk4imp, "Runge Kutta implicit order 4, Gauss' method", "rk4imp"},
  {bsimp, "Implicit Bulirsch-Stoer method of Bader and Deuflhard.", "bsimp"},
  {msadams, "Linear multistep Adams method in Nordsieck form.", "msadams"}, 
  {msbdf, "A variable-coefficient linear multistep backward differentiation formula (BDF) method in Nordsieck form.", "msbdf"}, 
  {-1}
  };


cgsl_integrator_named_ids cgsl_integrator_explicit_list[]={
  {rk2,	 "Runge Kutta order 2", "rk2"}, 
  {rk4,	 "Runge Kutta order 4", "rk4"}, 
  {rkf45,"Runge Kutta order 4 with Feldberg pair", "rkf45"}, 
  {rkck,  "Explicit embedded Runge-Kutta Cash-Karp (4, 5) method.", "rkck"}, 
  {rk8pd, "Runge Kutta order 8, Dormand Prince pair", "rk8pd"} , 
  {msadams, "Linear multistep Adams method in Nordsieck form.", "msadams"}, 
  {-1}
  };

cgsl_integrator_named_ids cgsl_integrator_implicit_list[]={
  {rk1imp, "Implicit Euler first order", "rk1imp"}, 
  {rk2imp, "Runge Kutta implicit order 2, Gauss's method", "rk2imp"}, 
  {rk4imp, "Runge Kutta implicit order 4, Gauss' method", "rk4imp"},
  {bsimp, "Implicit Bulirsch-Stoer method of Bader and Deuflhard.", "bsimp"},
  {msbdf, "A variable-coefficient linear multistep backward differentiation formula (BDF) method in Nordsieck form.", "msbdf"}, 
  {-1}
  };

cgsl_integrator_named_ids cgsl_integrator_rk_explicit_list[]={
  {rk2,	 "Runge Kutta order 2", "rk2"}, 
  {rk4,	 "Runge Kutta order 4", "rk4"}, 
  {rkf45,"Runge Kutta order 4 with Feldberg pair", "rkf45"}, 
  {rkck,  "Explicit embedded Runge-Kutta Cash-Karp (4, 5) method.", "rkck"}, 
  {rk8pd, "Runge Kutta order 8, Dormand Prince pair", "rk8pd"} , 
  {-1}
  };

cgsl_integrator_named_ids cgsl_integrator_rk_implicit_list[]={
  {rk1imp, "Implicit Euler first order", "rk1imp"}, 
  {rk2imp, "Runge Kutta implicit order 2, Gauss's method", "rk2imp"}, 
  {rk4imp, "Runge Kutta implicit order 4, Gauss' method", "rk4imp"},
  {-1}
  };

cgsl_integrator_named_ids cgsl_integrator_ms_explicit_list[]={
  {msadams, "Linear multistep Adams method in Nordsieck form.", "msadams"}, 
  {msbdf, "A variable-coefficient linear multistep backward differentiation formula (BDF) method in Nordsieck form.", "msbdf"}, 
  {-1}
  };

cgsl_integrator_named_ids cgsl_integrator_ms_implicit_list[]={
  {msbdf, "A variable-coefficient linear multistep backward differentiation formula (BDF) method in Nordsieck form.", "msbdf"}, 
  {-1}
};
