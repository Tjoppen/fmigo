#include "nonsmooth_clutch.h"



int main(){
 
  nonsmooth_clutch_params p ={
   {1.0,1, 1, 4000},           // masses
   1e-1 ,                       // first spring constant
   1e3 ,                        // second spring constant
   1e3 ,                        // pressure on plates
   1.0 ,                        // friction coefficient
   100 ,                        // torque on input shaft
   1,                           // torque on output shaft
   1.0 / 100.0,                 // time step
   2.0,                         // damping
   { -1, -1 },                  // lower limits
   { 1, 1 },                    // upper limits
   1e-10,                       // compliance 1
   1e-10,                       // compliance 2
   1e-10,                       // plate slip
   {0,0,0,0},                   // initial position
   {0,0,0,0}                    // initial velocity
 };

  void * sim = nonsmooth_clutch_init( p );

  nonsmooth_clutch_step( sim, (size_t) 1e6 );

  nonsmooth_clutch_free( sim );

  return 0;
}
