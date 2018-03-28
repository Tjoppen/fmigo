#define OCTAVE
#ifdef OCTAVE
#include <octave/oct.h>
#include <octave/ov-struct.h>
#endif

static const double scania_dx[]     = { -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 };
static const double scania_torque[] = { -1000, -30, 0, 50, 3500 };


/**
 *  Parameters should be read from a file but that's quicker to setup.
 *  These numbers were provided by Scania.
 */
static double fclutch( double dphi, double domega, double clutch_damping,
                       const double b[] = scania_dx, const double c[] = scania_torque,
                       size_t N = sizeof( scania_dx ) / sizeof( scania_dx[ 0 ] ) ) {
  
  //Scania's clutch curve

  size_t END = N-1;
    
  /** look up internal torque based on dphi
      if too low (< -b[ 0 ]) then c[ 0 ]
      if too high (> b[ 4 ]) then c[ 4 ]
      else lerp between two values in c
  */

  double tc = c[ 0 ]; //clutch torque

  if (dphi <= b[ 0 ]) {
    tc = (dphi - b[ 0 ]) / ( b[ 1 ] - b[ 0 ] ) *  ( c[ 1 ] - c[ 0 ] ) + c[ 0 ];
  } else if ( dphi >= b[ END ] ) {
    tc = ( dphi - b[ END ] ) / ( b[ END ] - b[ END - 1 ] ) * ( c[ END ] - c[ END - 1 ] ) + c[ END ];
  } else {
    int i;
    for (i = 0; i < END; ++i) {
      if (dphi >= b[ i ] && dphi <= b[ i+1 ]) {
	double k = (dphi - b[ i ]) / (b[ i+1 ] - b[ i ]);
	tc = (1-k) * c[ i ] + k * c[ i+1 ];
	break;
      }
    }
    if (i >= END ) {
      //too high (shouldn't happen)
      tc = c[ END ];
    }
  }

  //add damping. 
  tc += clutch_damping * domega;

  return tc; 

}

static double fclutch_domega_derivative( double dphi, double domega, double clutch_damping ) {

  return -clutch_damping;

}


static double fclutch_dphi_derivative( double dphi, double domega ) {
  
  //Scania's clutch curve
  static const double b[] = { -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 };
  static const double c[] = { -1000, -30, 0, 50, 3500 };
  size_t N = sizeof( c ) / sizeof( c[ 0 ] );
  size_t END = N-1;
    
  /** look up internal torque based on dphi
      if too low (< -b[ 0 ]) then c[ 0 ]
      if too high (> b[ 4 ]) then c[ 4 ]
      else lerp between two values in c
  */

  double df = 0;		// clutch derivative

  if (dphi <= b[ 0 ]) {
    df = 1.0 / ( b[ 1 ] - b[ 0 ] ) *  ( c[ 1 ] - c[ 0 ] );
  } else if ( dphi >= b[ END ] ) {
    df = 1.0 / ( b[ END ] - b[ END - 1 ] ) * ( c[ END ] - c[ END - 1 ] ) ;
  } else {
    int i;
    for (i = 0; i < END; ++i) {
      if (dphi >= b[ i ] && dphi <= b[ i+1 ]) {
	double k =  1.0  / (b[ i+1 ] - b[ i ]);
	df = k * ( c[ i + 1 ] -  c[ i ] );
	break;
      }
    }
    if (i >= END ) {
      //too high (shouldn't happen)
      df = 0;
    }
  }

  return df; 
}


DEFUN_DLD( fclutch, args, nargout, "clutch function"){
  octave_value_list retval;
  ColumnVector dphi     = static_cast<ColumnVector>(args(0).column_vector_value());
  ColumnVector domega   = static_cast<ColumnVector>(args(1).column_vector_value());
  double damping        = static_cast<double>(args(2).scalar_value());
  ColumnVector torque( dphi.length() );
  
  for ( size_t i = 0; i < dphi.length(); ++i ){
    torque( i ) = fclutch( dphi( i ), domega( i ), damping);
  }

  retval(0) = torque;
  
  return retval;
}


DEFUN_DLD( fclutch_dphi_derivative, args, nargout, "clutch angle derivative"){
  octave_value_list retval;
  ColumnVector dphi     = static_cast<ColumnVector>(args(0).column_vector_value());
  ColumnVector domega   = static_cast<ColumnVector>(args(1).column_vector_value());
  ColumnVector der( dphi.length() );
  
  for ( size_t i = 0; i < dphi.length(); ++i ){
    der( i ) = fclutch_dphi_derivative( dphi( i ), domega( i ) );
  }

  retval(0) = der;
  
  return retval;
}


DEFUN_DLD( fclutch_domega_derivative, args, nargout, "clutch speed derivative"){
  octave_value_list retval;
  ColumnVector dphi     = static_cast<ColumnVector>(args(0).column_vector_value());
  ColumnVector domega   = static_cast<ColumnVector>(args(1).column_vector_value());
  double damping        = static_cast<double>(args(2).scalar_value());
  ColumnVector der( dphi.length() );
  
  for ( size_t i = 0; i < dphi.length(); ++i ){
    der( i ) = fclutch_domega_derivative( dphi( i ), domega( i ), damping);
  }

  retval(0) = der;
  
  return retval;
}
