
#define OCTAVE
#ifdef OCTAVE
#include <octave/oct.h>
#include <octave/ov-struct.h>
#endif
#include <iostream>
#include <string>
using namespace std;



void print_oct( ColumnVector & x, string  n ){
  cerr << endl << "Octave vector : "  <<  n << endl;
  
  for ( size_t i = 0; i < x.length( ); ++i ) {
    cerr << n << "[ " << i << " ] = " << x( i ) << endl;
  }
  cerr << endl << endl ;
  

  return;
  
}


void print_array( double *  x, string  nm, size_t n ){

  cerr << endl << "Array : "  <<  nm << endl;
  
  for ( size_t i = 0; i < n; ++i ) {
    cerr << nm << "[ " << i << " ] = " << x[ i ] << endl;
  }
  
  cerr << endl << endl ;
  
}




/// fairly safe copy
void array_to_oct( double * x, ColumnVector & X ){
  memcpy(X.fortran_vec(),  x,   X.length() * sizeof( double ) );
}

/// less safe copy
void oct_to_array( ColumnVector & X, double *x ) {
  memcpy(x, X.fortran_vec(),    X.length() * sizeof( double ) );
}



#include <lumped_rod.h>

#if 0 
rod_sim * read_rod(const octave_value_list& args, int arg) {
  octave_scalar_map rod_struct = args(arg).scalar_map_value ();
  
  return sim;
}

#endif

DEFUN_DLD( rod_sim, args, nargout , "rod simulation") {
  octave_value_list retval;
  int arg = 0;
  size_t N = 1;
  
  
  octave_scalar_map rod_params = args(arg++).scalar_map_value ();
  octave_scalar_map init_state = args(arg++).scalar_map_value ();
  


  lumped_rod_sim_parameters p = {
      static_cast<double>(init_state.contents("step").scalar_value()),
    {

      static_cast<double>(init_state.contents("x1").scalar_value()),
      static_cast<double>(init_state.contents("xN").scalar_value()),
      static_cast<double>(init_state.contents("v1").scalar_value()),
      static_cast<double>(init_state.contents("vN").scalar_value()),
      static_cast<double>(init_state.contents("a1").scalar_value()),
      static_cast<double>(init_state.contents("aN").scalar_value()),
      static_cast<double>(init_state.contents("dx1").scalar_value()),
      static_cast<double>(init_state.contents("dxN").scalar_value()),
      static_cast<double>(init_state.contents("f1").scalar_value()),
      static_cast<double>(init_state.contents("fN").scalar_value()),
      static_cast<double>(init_state.contents("driver_f1").scalar_value()),
      static_cast<double>(init_state.contents("driver_fN").scalar_value()),
      static_cast<double>(init_state.contents("v_drive1").scalar_value()),
      static_cast<double>(init_state.contents("v_driveN").scalar_value()),

    },

    {

      static_cast<int>(rod_params.contents("N").scalar_value()),
      static_cast<double>(rod_params.contents("mass").scalar_value()),
      static_cast<double>(rod_params.contents("stiffness").scalar_value()),
      static_cast<double>(rod_params.contents("tau").scalar_value()),
      static_cast<double>(rod_params.contents("driver_stiffness1").scalar_value()),
      static_cast<double>(rod_params.contents("driver_relaxation1").scalar_value()),
      static_cast<double>(rod_params.contents("driver_stiffnessN").scalar_value()),
      static_cast<double>(rod_params.contents("driver_relaxationN").scalar_value())

    }
    
  };


  ColumnVector X( p.rod.n );        // positions
  ColumnVector V( p.rod.n );        // velocities
  ColumnVector T( p.rod.n-1 );      // for the torques 
  lumped_rod_sim sim = lumped_rod_sim_create( p ) ; 

  
  if ( args.length() > 2 ){ 
  X = static_cast<ColumnVector>( args( arg++ ).column_vector_value());
  oct_to_array( X, sim.rod.state.x );
}
  if ( args.length() > 3 ){ 
  V = static_cast<ColumnVector>( args( arg++ ).column_vector_value());
  oct_to_array( V, sim.rod.state.v );
}
  
  if ( args.length() > 4 )
    N  = static_cast<size_t>(  args( arg++ ).scalar_value());

  

  rod_sim_do_step( &sim, N );
  
  array_to_oct( sim.rod.state.x, X );
  array_to_oct( sim.rod.state.v, V );
  array_to_oct( sim.rod.state.torsion, T);
  
  lumped_rod_sim_free( sim );
  retval( 0 ) = X;
  retval( 1 ) = V;
  retval( 2 ) = T;

  return retval;
}






