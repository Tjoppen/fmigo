#include <fstream>
#include <sstream>
#include "CSV-parser.h"
using namespace std;

void fmigo_csv_printhelp(void){
      fprintf(stderr,"Error: fmigo CSV-parser.cpp expected format:\n");
      fprintf(stderr,"              --- #time,param1,param2,param3,... ---\n");
      fprintf(stderr,"              --- 0.0,tau1,tau2,tau3,...         ---\n");
      fprintf(stderr,"              --- 0.1,tau1,tau2,tau3,...         ---\n");
      fprintf(stderr,"              --- 0.2,tau1,tau2,tau3,...         ---\n");
      fprintf(stderr,"              ---  . , .. , .. , .. ,...         ---\n");
      fprintf(stderr,"              ---  . , .. , .. , .. ,...         ---\n");
      fprintf(stderr,"              ---  . , .. , .. , .. ,...         ---\n");
}
fmigo_csv_matrix fmigo_CSV_matrix(string csvf, char c){
  fmigo_csv_matrix matrix;
  ifstream file;
  file.open(csvf);

  string line;
  // extract inputnames
  if(getline(file, line)){
    istringstream head(line);

    if(line.at(0) == '#'){
      getline(head,line,'#');
      getline(head,line,c);
      while(getline(head,line,c))
        matrix.headers.push_back(line);
    }
    else{
      fmigo_csv_printhelp();
      exit(1);
    }
  }

  // fill the matrix
  while(getline(file, line)){
    istringstream head(line);
    string time;

    // first item in each row will be the key for that row
    getline(head,time,',');
    size_t i = 0;
    fmigo_csv_value params;
    while(getline(head, line,',')){
      if(i >= matrix.headers.size()){
        fprintf(stderr,"Error: fmigo CSV-parser.cpp: missmatching format - \"%s\" in file \"%s\"\n",head.str().c_str(), csvf.c_str());
        exit(1);
      }
      params[matrix.headers.at(i++)] = strtof(line.c_str(),0) ;
    }
    if(i != matrix.headers.size()){
      fprintf(stderr,"Error: fmigo CSV-parser.cpp: missmatching format - \"%s\" in file \"%s\"\n",head.str().c_str(), csvf.c_str());
      exit(1);
    }
    if(matrix.matrix.count(time)){
      fprintf(stderr,"Error: fmigo CSV-parser.cpp: keyname \"%s\" not uniq in file \"%s\" \n",time.c_str(), csvf.c_str());
      exit(1);
    }
    matrix.matrix[time] = params;
    matrix.time.push_back(time);
  }
  file.close();
  return matrix;
}

void printCSVmatrix(fmigo_csv_matrix m){

  fprintf(stderr,"time");
  for(auto c: m.headers)
        fprintf(stderr,", %s", c.c_str());
  fprintf(stderr,"\n");
  for(auto r: m.time){
    fprintf(stderr,"%s", r.c_str());
    for(auto c: m.headers){
      fprintf(stderr,", %f", m.matrix[r][c]);
    }
    fprintf(stderr,"\n");
  }


}
int main(){
  fmigo_csv_matrix m = fmigo_CSV_matrix("input.csv",',');
  printCSVmatrix(m);
  return 0;
}
