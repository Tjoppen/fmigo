/*
 * -----------------------------------------------------------------
 * $Revision: 4834 $
 * $Date: 2016-08-01 16:59:05 -0700 (Mon, 01 Aug 2016) $
 * -----------------------------------------------------------------
 * Programmer(s): Scott D. Cohen, Alan C. Hindmarsh and
 *                Radu Serban @ LLNL
 * -----------------------------------------------------------------
 * Example problem:
 *
 * The following is a simple example problem, with the coding
 * needed for its solution by CVODE. The problem is from
 * chemical kinetics, and consists of the following three rate
 * equations:
 *    dy1/dt = -.04*y1 + 1.e4*y2*y3
 *    dy2/dt = .04*y1 - 1.e4*y2*y3 - 3.e7*(y2)^2
 *    dy3/dt = 3.e7*(y2)^2
 * on the interval from t = 0.0 to t = 4.e10, with initial
 * conditions: y1 = 1.0, y2 = y3 = 0. The problem is stiff.
 * While integrating the system, we also use the rootfinding
 * feature to find the points at which y1 = 1e-4 or at which
 * y3 = 0.01. This program solves the problem with the BDF method,
 * Newton iteration with the CVDENSE dense linear solver, and a
 * user-supplied Jacobian routine.
 * It uses a scalar relative tolerance and a vector absolute
 * tolerance. Output is printed in decades from t = .4 to t = 4.e10.
 * Run statistics (optional outputs) are printed at the end.
 * -----------------------------------------------------------------
 */

#include <stdio.h>

/* Header files with a description of contents used */

#include  "/home/jonas/work/umit/umit-fmus/templates/gsl2/sundial-interface.h"
/* These macros are defined in order to write code which exactly matches
   the mathematical problem description given above.

   Ith(v,i) references the ith component of the vector v, where i is in
   the range [1..NEQ] and NEQ is defined below. The Ith macro is defined
   using the N_VIth macro in nvector.h. N_VIth numbers the components of
   a vector starting from 0.

   IJth(A,i,j) references the (i,j)th element of the dense matrix A, where
   i and j are in the range [1..NEQ]. The IJth macro is defined using the
   DENSE_ELEM macro in dense.h. DENSE_ELEM numbers rows and columns of a
   dense matrix starting from 0. */



/* Problem Constants */

#define NEQ   3                /* number of equations  */
#define Y1    RCONST(1.0)      /* initial y components */
#define Y2    RCONST(0.0)
#define Y3    RCONST(0.0)
#define RTOL  RCONST(1.0e-4)   /* scalar relative tolerance            */
#define ATOL1 RCONST(1.0e-8)   /* vector absolute tolerance components */
#define ATOL2 RCONST(1.0e-14)
#define ATOL3 RCONST(1.0e-6)
#define T0    RCONST(0.0)      /* initial time           */
#define T1    RCONST(0.4)      /* first output time      */
#define TMULT RCONST(10.0)     /* output time factor     */
#define NOUT  12               /* number of output times */


/* Functions Called by the Solver */

static int f(realtype t, N_Vector y, N_Vector ydot, void *user_data);

static int g(realtype t, N_Vector y, realtype *gout, void *user_data);

static int Jac(long int N, realtype t,
               N_Vector y, N_Vector fy, DlsMat J, void *user_data,
               N_Vector tmp1, N_Vector tmp2, N_Vector tmp3);

/* Private functions to output results */

static void PrintOutput(realtype t, realtype y1, realtype y2, realtype y3);
static void PrintRootInfo(int root_f1, int root_f2);

/* Private function to print final statistics */

static void PrintFinalStats(void *ode_mem);

/* Private function to check function return values */

static int check_flag(void *flagvalue, const char *funcname, int opt);


/*
 *-------------------------------
 * Main Program
 *-------------------------------
 */
void free_csundial_model(csundial_model * m){
  N_VDestroy_Serial(m->x);
  N_VDestroy_Serial(m->abstol);
}

int main()
{
    csundial_model model;
    csundial_step_control_parameters step_control;

    model.x = N_VNew_Serial(NEQ);
    model.neq = NEQ;
    if (check_flag((void *)model.x, "N_VNew_Serial", 0)) return (1);
    /* Initialize y */
    Ith(model.x,0) = Y1;
    Ith(model.x,1) = Y2;
    Ith(model.x,2) = Y3;

    model.abstol = N_VNew_Serial(NEQ);
    if (check_flag((void *)model.abstol, "N_VNew_Serial", 0)) return(1);
    /* Set the scalar relative tolerance */
    model.reltol = RTOL;
    /* Set the vector absolute tolerance */
    Ith(model.abstol,0) = ATOL1;
    Ith(model.abstol,1) = ATOL2;
    Ith(model.abstol,2) = ATOL3;

    model.function = f;
    model.jacobian = Jac;
    model.rootfinding = g;
    model.n_roots = 2;
    model.free = free_csundial_model;
    int rootsfound[model.n_roots];
    csundial_simulation sim = csundial_init_simulation(&model,cvode,0,NULL);


    /* In loop, call CVode, print results, and test for error.
     Break out of loop when NOUT preset output times have been reached.  */
    printf(" \n3-species kinetics problem\n\n");

    int iout = 0;  realtype tout = T1,t;
    int flag,flagr;
    while(1) {
        flag = csundial_step_to(&sim,sim.t,tout-sim.t);
        //flag = CVode(sim.ode_mem, tout, sim.model->x, &t, CV_NORMAL);
        PrintOutput(sim.t, Ith(sim.model->x,0), Ith(sim.model->x,1), Ith(sim.model->x,2));

        if (flag == CV_ROOT_RETURN) {
            flagr = csundial_get_roots(sim,rootsfound);
            //flagr = CVodeGetRootInfo(sim.ode_mem, rootsfound);
            if (check_flag(&flagr, "CVodeGetRootInfo", 1)) return(1);
            PrintRootInfo(rootsfound[0],rootsfound[1]);
        }

        if (check_flag(&flag, "CVode", 1)) break;
        if (flag == CV_SUCCESS) {
            iout++;
            tout *= TMULT;
        }

        if (iout == NOUT) break;
    }

    /* Print some final statistics */
    PrintFinalStats(sim.ode_mem);

    csundial_free_simulation(sim);
    return(0);
}


/*
 *-------------------------------
 * Functions called by the solver
 *-------------------------------
 */

/*
 * f routine. Compute function f(t,y).
 */

static int f(realtype t, N_Vector y, N_Vector ydot, void *user_data)
{
  realtype y1, y2, y3, yd1, yd3;

  y1 = Ith(y,0); y2 = Ith(y,1); y3 = Ith(y,2);

  yd1 = Ith(ydot,0) = RCONST(-0.04)*y1 + RCONST(1.0e4)*y2*y3;
  yd3 = Ith(ydot,2) = RCONST(3.0e7)*y2*y2;
        Ith(ydot,1) = -yd1 - yd3;

  return(0);
}

/*
 * g routine. Compute functions g_i(t,y) for i = 0,1.
 */

static int g(realtype t, N_Vector y, realtype *gout, void *user_data)
{
  realtype y1, y3;

  y1 = Ith(y,0); y3 = Ith(y,2);
  gout[0] = y1 - RCONST(0.0001);
  gout[1] = y3 - RCONST(0.01);

  return(0);
}

/*
 * Jacobian routine. Compute J(t,y) = df/dy. *
 */

static int Jac(long int N, realtype t,
               N_Vector y, N_Vector fy, DlsMat J, void *user_data,
               N_Vector tmp1, N_Vector tmp2, N_Vector tmp3)
{
  realtype y1, y2, y3;

  y1 = Ith(y,0); y2 = Ith(y,1); y3 = Ith(y,2);

  IJth(J,0,0) = RCONST(-0.04)*y1;
  IJth(J,0,1) = RCONST(1.0e4)*y3;
  IJth(J,0,2) = RCONST(1.0e4)*y2;
  IJth(J,1,0) = RCONST(0.04)*y1;
  IJth(J,1,1) = RCONST(-1.0e4)*y3-RCONST(6.0e7)*y2;
  IJth(J,1,2) = RCONST(-1.0e4)*y2;
  IJth(J,2,1) = RCONST(6.0e7)*y2;

  return(0);
}

/*
 *-------------------------------
 * Private helper functions
 *-------------------------------
 */

static void PrintOutput(realtype t, realtype y1, realtype y2, realtype y3)
{
#if defined(SUNDIALS_EXTENDED_PRECISION)
  printf("At t = %0.4Le      y =%14.6Le  %14.6Le  %14.6Le\n", t, y1, y2, y3);
#elif defined(SUNDIALS_DOUBLE_PRECISION)
  printf("At t = %0.4e      y =%14.6e  %14.6e  %14.6e\n", t, y1, y2, y3);
#else
  printf("At t = %0.4e      y =%14.6e  %14.6e  %14.6e\n", t, y1, y2, y3);
#endif

  return;
}

static void PrintRootInfo(int root_f1, int root_f2)
{
  printf("    rootsfound[] = %3d %3d\n", root_f1, root_f2);

  return;
}

/*
 * Get and print some final statistics
 */

static void PrintFinalStats(void *ode_mem)
{
  long int nst, nfe, nsetups, nje, nfeLS, nni, ncfn, netf, nge;
  int flag;

  flag = CVodeGetNumSteps(ode_mem, &nst);
  check_flag(&flag, "CVodeGetNumSteps", 1);
  flag = CVodeGetNumRhsEvals(ode_mem, &nfe);
  check_flag(&flag, "CVodeGetNumRhsEvals", 1);
  flag = CVodeGetNumLinSolvSetups(ode_mem, &nsetups);
  check_flag(&flag, "CVodeGetNumLinSolvSetups", 1);
  flag = CVodeGetNumErrTestFails(ode_mem, &netf);
  check_flag(&flag, "CVodeGetNumErrTestFails", 1);
  flag = CVodeGetNumNonlinSolvIters(ode_mem, &nni);
  check_flag(&flag, "CVodeGetNumNonlinSolvIters", 1);
  flag = CVodeGetNumNonlinSolvConvFails(ode_mem, &ncfn);
  check_flag(&flag, "CVodeGetNumNonlinSolvConvFails", 1);

  flag = CVDlsGetNumJacEvals(ode_mem, &nje);
  check_flag(&flag, "CVDlsGetNumJacEvals", 1);
  flag = CVDlsGetNumRhsEvals(ode_mem, &nfeLS);
  check_flag(&flag, "CVDlsGetNumRhsEvals", 1);

  flag = CVodeGetNumGEvals(ode_mem, &nge);
  check_flag(&flag, "CVodeGetNumGEvals", 1);

  printf("\nFinal Statistics:\n");
  printf("nst = %-6ld nfe  = %-6ld nsetups = %-6ld nfeLS = %-6ld nje = %ld\n",
	 nst, nfe, nsetups, nfeLS, nje);
  printf("nni = %-6ld ncfn = %-6ld netf = %-6ld nge = %ld\n \n",
	 nni, ncfn, netf, nge);
}

/*
 * Check function return value...
 *   opt == 0 means SUNDIALS function allocates memory so check if
 *            returned NULL pointer
 *   opt == 1 means SUNDIALS function returns a flag so check if
 *            flag >= 0
 *   opt == 2 means function allocates memory so check if returned
 *            NULL pointer
 */

static int check_flag(void *flagvalue, const char *funcname, int opt)
{
  int *errflag;

  /* Check if SUNDIALS function returned NULL pointer - no memory allocated */
  if (opt == 0 && flagvalue == NULL) {
    fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed - returned NULL pointer\n\n",
	    funcname);
    return(1); }

  /* Check if flag < 0 */
  else if (opt == 1) {
    errflag = (int *) flagvalue;
    if (*errflag < 0) {
      fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed with flag = %d\n\n",
	      funcname, *errflag);
      return(1); }}

  /* Check if function returned NULL pointer - no memory allocated */
  else if (opt == 2 && flagvalue == NULL) {
    fprintf(stderr, "\nMEMORY_ERROR: %s() failed - returned NULL pointer\n\n",
	    funcname);
    return(1); }

  return(0);
}
