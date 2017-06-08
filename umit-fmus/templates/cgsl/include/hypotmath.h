//This is a bit more involved version of the below fix:
//http://stackoverflow.com/questions/6809275/unresolved-external-symbol-hypot-when-using-static-library#10051898
//We need to do this in most binaries which is part of our project, but not all.
//So we can't just stick it in cgsl and be done, a gentler touch is required.
#ifdef WIN32
#ifdef _INC_MATH
#error Include hypotmath.h only. It also includes math.h for you
#endif

//make hypot in math.h get a different effective name
#define hypot inline_hypot
#define _USE_MATH_DEFINES //needed for M_PI
#include <math.h>
#undef hypot

//fix the link issue
double hypot(double x, double y) {return _hypot(x, y);}

#else
#include <math.h>
#endif
