#include <string.h>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <sstream>
#include "ioh5.h"
#include <H5EnumType.h>
#include <hdf5.h>
#include <getopt.h>
#include <utility>
#include <boost/regex.hpp>
#include <getopt.h>

using namespace boost;
using namespace std;
using namespace H5;
using namespace h5;

/** What this program is supposed to do: 

    01) open a destination file given in arg[1]
    -> should be done with -o file or --output file

    02) create a path for the images according to arg[2]: this is a '/'
    separated path, and starts at the root.  The path is constructed using
    the boost regex library to remove the slashes and create missing groups
    recursively. 

    03) open a file given as arg[3] 

    04) cleanup and exit. 

**/ 






int readit(string infile, string path, string name){

  H5File out(infile, H5F_ACC_RDONLY);
  
  /// double '/' doesn't matter but absence thereof isn't good
  Group root = out.openGroup(string("/")+path);
  string s;
  get_string(root, name, s);
  cout << s;
  root.close();
  out.close();
  
  return 0;
  
}


  
int insert(string infile, string outfile, string path, string name){

  ///
  /// The infile is utf8 text: just read it into a std string
  ///
  ifstream inFile;
  inFile.open(infile);          //open the input file
  stringstream strStream;
  strStream << inFile.rdbuf();//read the file
  string str = strStream.str();//str holds the content of the file 

  H5File out(outfile, H5F_ACC_RDWR);
  Group root = out.openGroup("/");
  Group o  = create_path(path, root);

  /// An overwrite option of sort here might be good.
  if (! hasChildNamed( o, name) ) {
      write_string_utf8(o, name, str);
    } else{
    /// construct a new name? 
  }
  out.close();
}

     
int
main (int argc, char **argv)
{
  int c;
  string path;
  string infile;
  string name;
  
  string usage("  -i,--input file -p,--path path -n,--name name  file\nThe input file must be encoded in utf8.  \nAnything else will produce garbage and may even core dump. "); 
     
  while (1)
  {
    static struct option long_options[] =
      {
        /* These options don't set a flag.
           We distinguish them by their indices. */
        {"input", required_argument,       0, 'i'},
        {"path",   required_argument, 0, 'p'},
        {"name",   required_argument, 0, 'n'},
        {0, 0, 0, 0}
      };
    /* getopt_long stores the option index here. */
    int option_index = 0;

     
    c = getopt_long (argc, argv, "i:p:n:", long_options, &option_index);
     
    /* Detect the end of the options. */
    if (c == -1)
      break;
     
    switch (c)
    {
      case 0:
        /* If this option sets a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        printf ("option %s", long_options[option_index].name);
        if (optarg){
          printf (" with arg %s", optarg);
        }
        printf ("\n");
        break;
     
      case 'p':
        path = string(optarg);
        break;
      case 'n':
        name = string(optarg);
        break;
      case 'i':
        infile = string(optarg);
        break;
     
      case '?':
        /* getopt_long already printed an error message. */
        break;
     
      default:
        abort ();
    }
  }
     
  
  string file(argv[optind++]);
  if ( file == string() || path ==  string() ){
    cerr << argv[0]  << usage << endl;
    return -1;
  }

  if (optind < argc) {
    cerr << argv[0]  << usage << endl;
    return -1;
  }

  if ( ! ( infile == string() ) ){
    insert(infile, file, path, name);
  }
  else {
    readit(file, path, name);
  }

  exit (0);

}
