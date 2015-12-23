/// The purpose of this little library is to, probably for the nth time,
/// abstract the OpenGL primitives and define shapes which can be rotated
/// and translated easily. 



#include <GL/glut.h>  // GLUT, include glu.h and gl.h
#include <GL/glu.h>  
#include <GL/gl.h>  
#include <vector>
#include <string.h>
#include <iostream>
using namespace std;

/// 
/// Color in OpenGL as defined with floats so it is important to make this
/// class translate everything ready for GL calls. 
/// 
struct color{
  float c[3];
  /// openGL is so standardized about data types that this is safe
  color() {
    memset( c, 0, 3 * sizeof ( float ) );
  }
  
  /// initialize with 3 scalars which can even be of different types.
  /// This is for the case where one writes (0, 0.0, 1.2f)  etc.
  template <typename R, typename S, typename T>
  color( R a, S b, T c ) : c{(float) a, (float) b, (float) c}{}

  template <typename T>
  /// initialize with an array of 3 values
  color( T _c[ 3 ] ) {
    for ( unsigned int i = 0; i < 3; ++i ) c[ i ] = (float)_c[ i ];
  }

  /// initialize with something indexable 

  template< class T > 
  color( const T & v ){
    for ( unsigned int i = 0; i < sizeof(c)/sizeof(c[0]); ++i ) c[ i ] = (float) v[ i ];
  }

  /// copy 
  color &  operator=( color _c ){
    for ( unsigned int i = 0; i < sizeof(c)/sizeof(c[0]); ++i ) c[ i ] = _c.c[ i ];
    return *this;
  }


  /// accessors. 
  
  float operator [] (size_t i)  const { return c[ i ]; }
  float & operator [] (size_t i)  { return c[ i ]; }
  float & r() { return c[ 0 ] ; }
  float & g() { return c[ 1 ] ; }
  float & b() { return c[ 2 ] ; }
  float   r() const  { return c[ 0 ] ; }
  float   g() const  { return c[ 1 ] ; }
  float   b() const  { return c[ 2 ] ; }


  
};

struct color_list {
  vector< color > colors;

  color_list(color c, size_t n) {
    colors.resize(n);
    for ( size_t i = 0; i < n; ++i ) colors[ i ] = c;
  }     // make n copies  of the same color

  color_list( const vector< color >&  _c ) :  colors( _c ){}

  color & operator[]( size_t i )       { return colors[ i ] ; }
  color   operator[]( size_t i ) const { return colors[ i ] ; }
  
  size_t  size() const { return colors.size(); }

};


struct vertex {
  float x;
  float y;
  vertex(): x(0), y(0){};
  template <typename T>
  vertex( float X[2] ): x( (float) X[ 0 ] ), y( ( float ) X[ 1 ] ){};
  vertex & operator= ( const vertex & v ) { x = v.x; y = v.y; return *this; }
  float operator [] (size_t i)  const {
    if ( i == 0 ) {
      return x;
    } else if ( i == 1 ){ 
      return y;
    }
    else {
        return 0; 
    }
  }
  
};

int main(){

  color c( {1,2,3} );

  color_list cl(c, 4);
  

  for ( size_t i = 0; i < 3; ++i )
    cerr << c[ i ] << "  " ;
  cerr << endl;

  cerr << cl.size() << endl;
  for ( size_t i = 0; i < cl.size(); ++i  ){
    for ( size_t j = 0; j < 3; ++j )
      cerr << cl[ i ][ j ] << "  " ;
    cerr << endl;
  }
  
  color_list cl1(
    {
    {1,3,4},
    {1,3,4},
    {1,3,4},
    {1,3,4},
    {1,3,4},
    {1,3,4}
    }
    
    );
  
  for ( size_t i = 0; i < cl1.size(); ++i  ){
    for ( size_t j = 0; j < 3; ++j ) { 
      cerr << cl1[ i ][ j ] << "  " ;
    }
    cerr << endl;
  }
#if 0 
#endif
  
    
  
  

  return 0;
  
}
