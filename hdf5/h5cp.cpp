#include <iostream>
#include <iomanip>
#include <string>
#include <stdio.h>
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
#include "ioh5.h"

using namespace boost;
using namespace std;
using namespace H5;
using namespace h5;


/// Read a whole file into a char * buffer : simpler in C than C++
int readin( const string & in, vector<char> & data){
  FILE * i = fopen(in.c_str(), "r");
  fseek(i, 0, SEEK_END);
  int length = ftell(i);
  fseek(i, 0, SEEK_SET);
  data.resize(length);
  fread(data.data(), sizeof(data[0]), data.size(), i);
  fclose(i);
  return 0;
  
}

//
//  Write a char * buffer to a file
//
int writeout( const string & out, const vector<char> & data ) {
  
  FILE *o = fopen(out.c_str(), "w");
  fwrite(data.data(), sizeof(data[0]), data.size(), o);
  fflush(o);
  fclose(o);
  return 0;
  
}

///
///  Dumps a whole file into an hdf5 file
///
int h5_insert_file(const string &in, const string& out, const string & path, const string & name)
{ 
  vector<char> buffer;
  
  readin(in, buffer);
  
  H5File h5out(out, H5F_ACC_RDWR);
  Group g = h5out.openGroup(path);
  write_vector(g, name, buffer);
  h5out.close();
  return 0;
  
}

int h5_fetch_file(const string &in, const string& out,
                  const string & from_path, const string & from_name)
{
  vector<char>  buffer;
  
  H5File i(in, H5F_ACC_RDONLY);
  Group ingroup = i.openGroup(from_path); 
  read_vector(ingroup, from_name, buffer);
  ingroup.close();
  writeout( out,  buffer);
}



int main() {

  vector<char>  buffer;

  h5_insert_file("tt.h5", "tth5cp.h5", "/", "file");
  h5_fetch_file("tth5cp.h5", "ttcpround.h5", "/", "file" );
  

  
  return 0;
  
}
