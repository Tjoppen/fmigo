#include "ioh5.h"
#include <iostream>
#include <sstream>
#include <valarray>
#include <stdio.h>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>
using namespace std;
using namespace H5;
using namespace h5;




int main()
{
  vector<int>  x({1,2,3,4,5,6,7,7});
  vector<double>  y({1,2,3,4,5,6,7,7});
  
  H5File dest("testmea.h5", H5F_ACC_TRUNC);
  Group g = dest.openGroup("/");
  
  write_string(g,  "s", string("foobar"));
  write_vector(g,  string("a"), x, 1);
  write_vector(g,  "b", y, 2);

  g.close();
  
  dest.close();

  
  return 0;
}
