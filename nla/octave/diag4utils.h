#include"diag4.h"
#include <iostream>
using namespace std;




/// Utilities to copy valarray to and from octave vectors


inline void oct_to_val( const ColumnVector & x, std::valarray<Real> & y ){
  if ( x.length() != y.size() ) y.resize( x.length() );
  memcpy(&y[ 0 ], x.fortran_vec(), y.size() * sizeof( y[ 0 ] ) );
}

inline void val_to_oct( const std::valarray<Real> & y, ColumnVector & x ){
  memcpy(x.fortran_vec(), &y[ 0 ], y.size() * sizeof( y[ 0 ] ) );
}

inline void val_to_oct_int( const std::valarray<int> & y, ColumnVector & x ){
  for ( size_t i = 0; i < y.size();  ++i )
    x( i )  = (double) y[ i ];
}

inline void oct_to_val_int(  const ColumnVector & x , std::valarray<int> & y){
  if ( x.length() != y.size() ) y.resize( x.length() );
  for ( size_t i = 0; i < y.size();  ++i )
    y[ i ]  = (int)x( i );

}

///
///  The semantic here is that the `_active'  array is *block-wise* since
///  only odd variables can in fact be deactivated.
///

inline diag4::diag4(ColumnVector _diag, ColumnVector _sub, ColumnVector _ssub,
                    ColumnVector _fill, ColumnVector _active, bool _bisymmetric) :
  diag4( (size_t) _diag.length() / 2, _bisymmetric  ) // diag4 constructor needs number of *blocks*
{
  oct_to_val(_diag, diag);
  oct_to_val(_sub, sub);
  oct_to_val(_ssub,ssub);
  oct_to_val(_fill, fill);

  for ( size_t i = 0 ; i < n_blocks ; ++i ){
    active[ 2 * i     ] = true;
    active[ 2 * i + 1 ] = ( bool ) _active( i );
  }
   sync();
}

inline void diag4::read_factor(const octave_value_list& args, int arg) {

  octave_scalar_map diag_struct = args(arg).scalar_map_value ();
  ColumnVector  _diag = static_cast<ColumnVector>(diag_struct.contents(string("diag")).column_vector_value());
  ColumnVector  _sub = static_cast<ColumnVector>(diag_struct.contents(string("sub")).column_vector_value());
  ColumnVector  _ssub = static_cast<ColumnVector>(diag_struct.contents(string("ssub")).column_vector_value());
  ColumnVector  _fill = static_cast<ColumnVector>(diag_struct.contents(string("fill")).column_vector_value());
  oct_to_val(_diag, diag);
  oct_to_val(_sub, sub);
  oct_to_val(_ssub, ssub);
  oct_to_val(_fill, fill);
}


inline void diag4length::read_factor(const octave_value_list& args, int arg) {
}

////
/// A wrapper for the previous constructor.  Transforms a struct to a diag4
/// matrix

inline qp_diag4 * read_matrix(const octave_value_list& args, int arg) {
  octave_scalar_map diag_struct = args(arg).scalar_map_value ();
  ColumnVector  diag   = static_cast<ColumnVector>(diag_struct.contents(string("diag")).column_vector_value());
  ColumnVector  sub    = static_cast<ColumnVector>(diag_struct.contents(string("sub")).column_vector_value());
  ColumnVector  ssub   = static_cast<ColumnVector>(diag_struct.contents(string("ssub")).column_vector_value());
  ColumnVector  fill   = static_cast<ColumnVector>(diag_struct.contents(string("fill")).column_vector_value());
  ColumnVector  active = static_cast<ColumnVector>(diag_struct.contents(string("active")).column_vector_value());

  octave_value tmp = diag_struct.contents(string("bisymmetric"));
  bool bisymmetric     = false;


  if ( tmp.is_defined( ) ){
    bisymmetric = (bool) tmp.scalar_value();
  }

  octave_value jj = diag_struct.contents(string("J"));
  if ( jj.is_defined( ) ){
    ColumnVector  J = static_cast<ColumnVector>(diag_struct.contents(string("J")).column_vector_value());
    return  new diag4length(diag, sub, ssub, fill, J,  active, bisymmetric) ;
  } else {
    return  new diag4(diag, sub, ssub, fill, active, bisymmetric) ;
  }
}

///
///  This exports both the original and the factored matrix
///
inline void diag4::convert_matrix_oct(octave_scalar_map & st) {

  ColumnVector _diag(diag.size());
  val_to_oct( diag, _diag );
  st.assign("diag", _diag);
  val_to_oct( original.diag, _diag );
  st.assign("diag0", _diag);

  ColumnVector _sub(sub.size());
  val_to_oct( sub, _sub );
  st.assign("sub", _sub);
  val_to_oct( original.sub, _sub );
  st.assign("sub0", _sub);

  ColumnVector _ssub(ssub.size());
  val_to_oct( ssub, _ssub );
  st.assign("ssub", _ssub);
  val_to_oct( original.ssub, _ssub );
  st.assign("ssub0", _ssub);

  ColumnVector _fill(fill.size());
  val_to_oct( fill, _fill );
  st.assign("fill", _fill);

  /// NOTE: this is really bad semantics because active is variable-wise in
  /// the library but blockwise outside.   This means in particular that it
  /// is not possible to deactivate the last equation in the testing
  /// framework as it stands.
  ColumnVector _active(diag.size()/2);
  for ( int i = 0; i < _active.length() / 2; ++i)
    _active( i ) = (double)active[ 2 * i + 1 ];
  st.assign("active", _active);
  st.assign("bisymmetric", bisymmetric);
}

inline void diag4length::convert_matrix_oct(octave_scalar_map& st){
  diag4::convert_matrix_oct(st );
  ColumnVector _J(J.size());
  val_to_oct(J, _J );
  st.assign("J", _J);
  val_to_oct(J0, _J );
  st.assign("J0", _J);
  st.assign("i_schur", i_schur);
}


inline diag4length::diag4length(ColumnVector _diag, ColumnVector _sub, ColumnVector _ssub,
                                ColumnVector _fill, ColumnVector _J, ColumnVector _active, bool bisymmetric) :
  diag4( _diag, _sub, _ssub, _fill, _active, bisymmetric)
{
  valarray<bool>  _a( active );

  J0.resize( _J.length() );
  oct_to_val(_J, J0);
  J = J0;
  J[ J.size() - 1 ] = Real(0.0);
  active.resize( J0.size() );
  for ( size_t i = 0; i < _a.size(); ++i) active[ i ] = _a[ i ];
  active[ active.size() - 1 ] = true;
  sync();
}
