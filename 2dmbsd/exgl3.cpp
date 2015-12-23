/*
 * GL05IdleFunc.cpp: Translation and Rotation
 * Transform primitives from their model spaces to world space (Model Transform).
 */
#include <GL/glut.h>  // GLUT, include glu.h and gl.h
#include <GL/glu.h>  
#include <GL/gl.h>  
#include <vector>
#include <string.h>
using namespace std;

struct color{
  float c[3];
  color() {
    for ( unsigned int i = 0; i < sizeof(c)/sizeof(c[0]); ++i ) c[ i ] = 0;
  }
  template <typename T>
  color( T a, T b, T c ) : c({a,b,c}){}
  template <typename T>
  color( T _c[ 3 ] ) {
    for ( unsigned int i = 0; i < sizeof(c)/sizeof(c[0]); ++i ) c[ i ] = (float)_c[ i ];
  }
  color &  operator=( color _c ){
    for ( unsigned int i = 0; i < sizeof(c)/sizeof(c[0]); ++i ) c[ i ] = _c.c[ i ];
    return *this;
  }
  float operator [] (size_t i)  const { return c[ i ]; }
};

struct color_list {
  vector< color > colors;
  

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


struct gl_shape {
  GLenum shape;
  vector< vertex > vertices;    // we can color each vertices
  vector< color > colors;    // we can color each vertices
  gl_shape() : shape( 0 ) , vertices(0), colors(0) {}

  gl_shape(GLenum _shape, const vector< vertex >&  _vertices, const vector< color >& _colors  ) :  shape( _shape ), vertices( _vertices ), colors( _colors ) {}
  virtual void drawme( ){

    for ( size_t i = 0; i < vertices.size(); ++i ){
      glColor3f( colors[ i ][ 0 ], colors[ i ][ 1 ], colors[ i ][ 2 ]);
      glVertex2f( vertices[ i ][ 0 ], vertices[ i ][ 1 ] );
    }

  }

  /// There is a problem here because one might be tempted to give an array
  /// of doubles for instance. 

  template <typename T > 
  void draw( T q[ 3 ] ){
    float p[3] = {(float) q[ 0 ], (float) q[ 1 ], (float) q[ 2 ]};
    draw( p );
  }
  
  
  virtual void draw( float q[ 3 ] ){
    glPushMatrix();                     // Save model-view matrix setting
    glTranslatef(( float ) q[ 0 ], ( float ) q[ 1 ], 0.0f);    // Translate
    glRotatef( ( float ) q[ 3 ], 0.0f, 0.0f, 1.0f); // rotate by angle in *degrees*
    glBegin(shape);                  // Each set of 4 vertices form a quad
    
    drawme();
    
    glEnd();                  // Each set of 4 vertices form a quad
    glPopMatrix();                      // Restore the model-view matrix
  }

};


struct gl_rectangle : gl_shape {

  template < typename T, typename U > 

  gl_rectangle( vector< color > & c,  U _l1, U _l2 )   {
    
    gl_shape(GL_QUADS, color,
             vector< vertex > (
               {
                   vertex( -_l1, -_l2),
                   vertex(  _l1, -_l2), 
                   vertex(  _l1,  _l2), 
                   vertex( -_l1,  _l2)
                   } ) );
  }

                                                 };



// Global variable
GLfloat angle = 0.0f;  // Current rotational angle of the shapes
 
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

/* Handler for window-repaint event. Call back when the window first appears and
   whenever the window needs to be re-painted. */
  vector <gl_shape * > shapes;

  simulation() {
    shapes.push_back( new gl_rectangle( color(1,0,0), 0.2, 0.2 ) );
  }
  

static void display() {
  glClear(GL_COLOR_BUFFER_BIT);   // Clear the color buffer
  glMatrixMode(GL_MODELVIEW);     // To operate on Model-View matrix
  glLoadIdentity();               // Reset the model-view matrix

  float color[] = {1,1,0};
  float pos[] = {-0.5, 0.5, 0 };
  for ( size_t i = 0; i < shapes.size(); ++i ) {
    shapes[ i ] -> draw( );
  }
  glutSwapBuffers();   // Double buffered - swap the front and back buffers
 
  // Change the rotational angle after each display()
  angle += 0.2f;
}
};



  
 
/* Handler for window re-size event. Called back when the window first appears and
   whenever the window is re-sized with its new width and height */
void reshape(GLsizei width, GLsizei height) {  // GLsizei for non-negative integer
  // Compute aspect ratio of the new window
  if (height == 0) height = 1;                // To prevent divide by 0
  GLfloat aspect = (GLfloat)width / (GLfloat)height;
 
  // Set the viewport to cover the new window
  glViewport(0, 0, width, height);
 
  // Set the aspect ratio of the clipping area to match the viewport
  glMatrixMode(GL_PROJECTION);  // To operate on the Projection matrix
  glLoadIdentity();
  if (width >= height) {
    // aspect >= 1, set the height from -1 to 1, with larger width
    gluOrtho2D(-1.0 * aspect, 1.0 * aspect, -1.0, 1.0);
  } else {
    // aspect < 1, set the width to -1 to 1, with larger height
    gluOrtho2D(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect);
  }
}
 
/* Main function: GLUT runs as a console application starting at main() */
int main(int argc, char** argv) {
  simulation sim;
  
  glutInit(&argc, argv);          // Initialize GLUT
  glutInitDisplayMode(GLUT_DOUBLE);  // Enable double buffered mode
  glutInitWindowSize(640, 480);   // Set the window's initial width & height - non-square
  glutInitWindowPosition(50, 50); // Position the window's initial top-left corner
  glutCreateWindow("Animation via Idle Function");  // Create window with the given title


  


  glutDisplayFunc(sim.display);       // Register callback handler for window re-paint event
  
  glutReshapeFunc(reshape);       // Register callback handler for window re-size event
  glutIdleFunc(idle);             // Register callback handler if no other event
  initGL();                       // Our own OpenGL initialization
  glutMainLoop();                 // Enter the infinite event-processing loop
  return 0;
}
