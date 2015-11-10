#include "ioh5.h"
#include <iostream>
#include <valarray>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>
#include <hdf5.h>
#include <sstream>              // for string manipulations.
#include <iomanip>
#include <iostream>
#include<initializer_list>
#include <utility>
#include <boost/regex.hpp>
#include <getopt.h>

using namespace boost;
using namespace std;
using namespace H5;
using namespace h5;


int main(){

  H5File in("../data/agx/sliding/sliding-box.journal", H5F_ACC_RDONLY); //
  Group g = in.openGroup("/Sessions/2015.06.22-23.45.39-159770/000001/RigidBody/velocity");
  vector<double> w;
   read_array(g, string("data"), w);
   H5File out("myout.h5", H5F_ACC_TRUNC); //
   Group go = out.openGroup("/");
   write_vector(go, string("velocities"), w);
   go.close();
   out.close();
   return 0;
   

}
