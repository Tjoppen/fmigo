#include <iostream>
using namespace std;

struct foo{
  int i;
  foo * f;
  foo( int j, bool first = false ) : i( j ), f( 0 ) {
    if (! first  )
      f = new foo( j );
  }
  ~foo() {
    if ( f )
      delete f;
    
  }

};


int main(){
  size_t  i= 1;
  size_t  j = 2;
  size_t c = i-j;
  size_t d = (size_t) -1 ;
  
  
  cerr <<  c << "  "  <<  d  << "   " << c-d << endl;
  
  return 0;
  

}
