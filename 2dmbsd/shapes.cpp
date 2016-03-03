/// The purpose of this little library is to, probably for the nth time,
/// abstract the OpenGL primitives and define shapes which can be rotated
/// and translated easily. 



#include <GL/glut.h>  // GLUT, include glu.h and gl.h
#include <GL/glu.h>  
#include <GL/gl.h>  
#include <vector>
#include <string.h>
#include <iostream>
#include <math.h>
using namespace std;

/// 
/// Color in OpenGL as defined with floats so it is important to make this
/// class translate everything ready for GL calls. 
/// 
struct color{
  float r;
  float g;
  float b;
  
  /// openGL is so standardized about data types that this is safe
  color() {
    r = 0;
    g = 0;
    b = 0;
  }

  
  /// initialize with 3 scalars which can even be of different types.
  /// This is for the case where one writes (0, 0.0, 1.2f)  etc.
  template <typename R, typename S, typename T>
  color( R a, S b, T c ) :  r( ( float ) a), g( ( float ) b ), b( ( float ) c ){}

  template <typename T>
  /// initialize with an array of 3 values
  color( T _c[ 3 ] ) {
    r =  (float)_c[ 0 ];
    g =  (float)_c[ 1 ];
    b =  (float)_c[ 2 ];
  }

  /// initialize with something indexable 

  template< class T > 
  color( const T & v ){
    r = v[ 0 ];
    g = v[ 1 ];
    b = v[ 2 ];
  }

  /// copy 
  color &  operator=( color _c ){
    r = _c.r;
    g = _c.g;
    b = _c.b;
    return *this;
  }


  /// accessors. 
  
  float operator [] ( size_t i ) const { return ( ( float [ 3 ]) {r,g,b})[i]; }
  float & operator [] ( size_t i ) { return      * ( ( float * )this  + i ); }
  float & red   () { return r ; }
  float & green () { return g ; }
  float & blue  () { return b ; }

  float   red   ()  const { return r ; }
  float   green ()  const { return g ; }
  float   blue  ()  const { return b ; }

  
};

typedef vector< color > color_list;


struct vertex {
  float x;
  float y;
  vertex(): x(0), y(0){};
  template <typename T, typename U>
  vertex( T _x, U _y ) : x( (float ) _x ), y( ( float ) _y )  {}


  template <class T >
  vertex( T X ) : x( ( float ) X[ 0 ] ), y( ( float ) X[ 1 ] ){};

  vertex & operator= ( const vertex & v ) { x = v.x; y = v.y; return *this; }
  float operator [] ( size_t i ) const { return * ( ( float * ) this  + i ) ; }
  float & operator [] ( size_t i ) { return  * ( ( float * ) this  + i ) ; }

};

typedef vector< vertex > vertex_list;
typedef float scalar;
typedef float vec2[ 2 ];

float rad_to_degre( float x ) { return 180.0f * x / M_PI; }
float degre_to_rad( float x ) { return M_PI *  x / 180.0f; } 

struct coord2d {
  vec2  x;
  float phi;
};



struct gl_shape {
  GLenum shape;
  vertex_list vertices;    // we can color each vertices
  color_list colors;    // we can color each vertices
  coord2d q;

  void translate( vec2  x ){
    q.x( x );
    
  }
  
  gl_shape() : shape( 0 ) , vertices(), colors() {}

  gl_shape(GLenum _shape, vertex_list   _vertices, color_list  _colors  ) :
    shape( _shape ), vertices( _vertices ), colors( _colors ) {}

  gl_shape(GLenum _shape, const vertex_list &  _vertices, color c ) :
    shape( _shape ), vertices( _vertices ), colors( _vertices.size(), c ) {}

  virtual void drawme( ){
    for ( size_t i = 0; i < vertices.size(); ++i ){
      glColor3f( colors[ i ].red(), colors[ i ].green(), colors[ i ].blue());
      glVertex2f( vertices[ i ].x, vertices[ i ].y );
    }
  }
  

  /// There is a problem here because one might be tempted to give an array
  /// of doubles for instance. 

  /// subclasses are responsible to use this function
  virtual void draw( ){
    glPushMatrix();                     // Save model-view matrix setting
    glTranslatef( q.x[ 0 ], q.x[ 1 ], 0.0f);    // Translate
    glRotatef(  q.phi, 0.0f, 0.0f, 1.0f); // rotate by angle in *degrees*
    glBegin(shape);                  // Each set of 4 vertices form a quad
    
    drawme();
    
    glEnd();                  // Each set of 4 vertices form a quad
    glPopMatrix();                      // Restore the model-view matrix
  }

};

typedef vector< gl_shape* > gl_shape_list;


struct gl_rectangle : public gl_shape {
  template < typename T, typename U > 
  gl_rectangle( T l1,  U l2 ) : gl_rectangle( l1,  l2, color() ) {} 
  template < typename T, typename U > 
  gl_rectangle( T l1,  U l2 , color  c) : 
    gl_shape(GL_QUADS,
             vertex_list ({
                 vertex( ( float ) -l1, ( float ) -l2),
                 vertex( ( float )  l1, ( float ) -l2), 
                 vertex( ( float )  l1, ( float )  l2), 
                 vertex( ( float ) -l1, ( float )  l2)
                   }), color_list( 4, c  ) ) {}
};

struct gl_square : public gl_rectangle {

  template < typename T > 
  gl_square( T l1 ) : gl_rectangle( l1,  l1, color() ) {} 

  template < typename T > 
  gl_square( T l1,  color  c) : gl_rectangle( l1, l1, c) {}
  
};

/* Initialize OpenGL Graphics */
void initGL() {
   // Set "clearing" or background color
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black and opaque
}

/* Called back when there is no other event to be handled */
void idle() {
   glutPostRedisplay();   // Post a re-paint request to activate display()
}

struct simulation {

  static gl_shape_list shapes;
  
/* Handler for window-repaint event. Call back when the window first appears and
   whenever the window needs to be re-painted. */
  
  simulation() {
    shapes.push_back( new gl_rectangle( 0.2, 0.4, color(1,0,0)) );
  }
  
  static void pre(){
    return;
    
  }
  static void post(){
  }

  static void display() {
    pre();
    
    float angle = 0.0f;
    glClear(GL_COLOR_BUFFER_BIT);   // Clear the color buffer
    glMatrixMode(GL_MODELVIEW);     // To operate on Model-View matrix
    glLoadIdentity();               // Reset the model-view matrix
    
    color c(1,1,0);
    for ( size_t i = 0; i < shapes.size(); ++i ) {
      shapes[ i ] -> draw( );
  }
    glutSwapBuffers();   // Double buffered - swap the front and back buffers
 
    post();
    // Change the rotational angle after each display()
    angle += 0.2f;
  }
};



int main()
{
  gl_rectangle r( 1.0, 3.0, color(1,0,0) );
  return 0;
}
