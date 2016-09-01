#ifndef DIAG4_TYPES
#define DIAG4_TYPES

#ifndef AGXDATA_REAL_H
typedef double Real;
#else
using namespace agx;
#endif
/// \todo Added for things to compile. Should probably be something else.
struct steps
{
  Real theta0;
  Real theta1;

  enum states
  {
    LOWER = 0,
    UPPER = 1,
  };

  void update(Real, int, int) {}
};

#endif
