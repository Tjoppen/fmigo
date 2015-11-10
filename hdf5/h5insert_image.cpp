#include <string.h>
#include <iostream>
#include <typeinfo>
#include <vector>
#include <valarray>
#include "ioh5.h"
#include <H5EnumType.h>
#include <hdf5.h>
#include <utility>
#include <boost/regex.hpp>
#include <getopt.h>

using namespace std;
using namespace H5;
using namespace h5;
using namespace boost;

/** What this program is supposed to do: 

    01) open a destination file given in arg[1]
    -> should be done with -o file or --output file

    02) create a path for the images according to arg[2]: this is a '/'
    separated path, and starts at the root.  The path is constructed using
    the boost regex library to remove the slashes and create missing groups
    recursively. 

    03) open a list of files given as arg[3] ....  In each, 
    check that there is a dataset with attribute IMAGE_SUBCLASS.
    If this was created with gif2h5, which is assumed for now, then there's
    only one dataset called "Image0".  Rename all these pics from 000000
    000001 etc. 

    04) cleanup and exit. 

**/ 




typedef std::map<std::string, int, std::less<std::string> > map_type; 


Group create_path(const std::string& names, Group g) 
{ 
  map_type m;
  std::string::const_iterator start, end; 
  start = names.begin(); 
  end = names.end(); 
  regex expression("([^/])+[/]?");
  match_results<std::string::const_iterator> what; 
  match_flag_type flags = match_default; 
  Group o = g; 

  while(regex_search(start, end, what, expression, flags)) 
  { 
    string s = remove_slash(string(what[0]));
    o = getOrCreateGroup(o, s);
    // update search position: 
    start = what[0].second; 
    // update flags: 
    flags |= boost::match_prev_avail; 
    flags |= boost::match_not_bob; 
  } 
  return o; 
}

bool hasChild(Group & g, string child){
  bool ret = false; 
  try { 
    Exception::dontPrint();
    Group c =  g.openGroup(child);
    c.close();
    ret = true; 
  } 
  catch (H5::GroupIException& error ) { 
  }
  return ret;
}

bool hasAttribute(H5Object & o, string n)
{
  bool ret = false; 
  try { 
    Exception::dontPrint();
    Attribute a =  o.openAttribute(n);
    a.close();
    ret = true; 
  } 
  catch (H5::AttributeIException& error ) { 
  }
  return ret;
}

Group getGroupOrCreate(Group & g, string c)
{
  Group o; 
  if (hasChild(g, c)){
    o = g.openGroup(c);
  } else {
    o = g.createGroup(c);
  }
  
  return o; 
}



  
int insert(string infile, string outfile, string path){


  H5File src(infile, H5F_ACC_RDONLY); 
  H5File out(outfile, H5F_ACC_RDWR);

  Group o  = create_path(path, out.openGroup("/"));
  //
  // this will simply die if there is no Image0 dataset */
  // MUST ADD ABILITY TO CREATE THE GROUP AND ALSO INSERT SEVERAL IMAGES
  //
  DataSet i = src.openGroup("/").openDataSet("Image0");

  if ( hasAttribute(i, "IMAGE_SUBCLASS")  ) {

    hid_t ocpypl_id = H5Pcreate(H5P_OBJECT_COPY);
    H5Pset_copy_object(ocpypl_id, H5P_DEFAULT); 
    H5Ocopy(i.getId(), "/", o.getId(), "frame", ocpypl_id, H5P_DEFAULT);
    H5Lmove(o.openGroup("frame").getId(), "Image0", H5L_SAME_LOC, "frame", H5P_DEFAULT,
            H5P_DEFAULT);
  }
}

     
int
main (int argc, char **argv)
{
  int c;
  string out; 
  string path;
  string files;
  bool append = false;
  string usage("  -o,--output file -p,--path path  infile ");

     
  while (1)
  {
    static struct option long_options[] =
      {
        /* These options don't set a flag.
           We distinguish them by their indices. */
        {"output", required_argument,       0, 'o'},
        {"path",   required_argument, 0, 'p'},
        {0, 0, 0, 0}
      };
    /* getopt_long stores the option index here. */
    int option_index = 0;

     
    c = getopt_long (argc, argv, "ao:p:",
                     long_options, &option_index);
     
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
     
      case 'a':
        puts ("option -a\n");
        break;
     
      case 'o':
        printf ("option -o with value `%s'\n", optarg);
        out = string(optarg);
        break;
     
      case 'p':
        printf ("option -p with value `%s'\n", optarg);
        path = string(optarg);
        break;
     
      case '?':
        /* getopt_long already printed an error message. */
        break;
     
      default:
        abort ();
    }
  }
     
  if ( out == string() || path ==  string() ){
    cerr << argv[0]  << usage << endl;
    return -1;
  }
  
     
  /* Print any remaining command line arguments (not options). */
  string infile(argv[optind++]);
  if (optind < argc)
  {
    cerr << argv[0]  << usage << endl;
    return -1;
  }

  insert(infile, out, path);
     
  exit (0);
}
