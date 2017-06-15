#include "lumped_rod_io.h"
#include <string>


/**
 * Open a file, get a new unique problem ID, 
 */
string write_lumped_rod_config( const char * filename, const char * groupname, lumped_rod_sim * sim){

  H5File * f = append_or_create( filename ) ; 
  Group g0 = append_problem(f, "lumped_rod");

  sim->h5_group_id = ( size_t ) g0.getLocId();

  Group g = g0.openGroup("configuration");
  write_scalar( g, "mass", sim->rod.rod_mass);
  write_scalar( g, "N", (double) sim->rod.n);
  write_scalar( g, "compliance", (double) sim->rod.compliance);
  write_scalar( g, "tau",  sim->tau);
  write_array( g, "mobility",  sim->rod.mobility, 4);
  write_scalar( g, "step",  sim->step);
  write_scalar( g, "x1",  sim->state.x1);
  write_scalar( g, "xN",  sim->state.xN);
  write_scalar( g, "v1",  sim->state.v1);
  write_scalar( g, "vN",  sim->state.vN);
  write_scalar( g, "f1",  sim->state.f1);
  write_scalar( g, "fN",  sim->state.fN);
  
  g.close();
  g0.cose();
  f->close();
  
  return;

}


void write_lumped_rod_states( const char * filename, const char * groupname, lumped_rod_sim * sim){


  return;
  
}
