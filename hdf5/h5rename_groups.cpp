/************************************************************

  This example shows how to recursively traverse a file
  using H5Ovisit and H5Lvisit.  The program prints all of
  the objects in the file specified in FILE, then prints all
  of the links in that file.  

  This file is intended for use with HDF5 Library version 1.8

************************************************************/

#include "hdf5.h"
#include <stdio.h>
#include <iostream>
#include <utility>
#include <getopt.h>
using namespace std;




/* * Operator function to be called by H5Lvisit. */
herr_t op_rename_link (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data);

typedef   pair<string, string>  iostrings ;

int
rename_groups (string F, string from , string to )
{
  iostrings io= {from, to };
  

  
  hid_t file = H5Fopen (F.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);

  /// for reasons beyond my understanding, the iteration operations are not applied at the root node.
  if ( H5Lexists( file, io.first.c_str(), H5P_DEFAULT ) ) {
    H5Lmove( file, io.first.c_str(), file, io.second.c_str(), H5P_DEFAULT, H5P_DEFAULT );
  }

  hid_t status = H5Lvisit (file, H5_INDEX_NAME, H5_ITER_NATIVE, op_rename_link, (void*)&io);
  status = H5Fclose (file);

  return status;
}




/************************************************************

  Operator function for H5Lvisit.  This function simply
  retrieves the info for the object the current link points
  to, and calls the operator function for H5Ovisit.

************************************************************/
herr_t op_rename_link (hid_t loc_id, const char *name, const H5L_info_t *info, void *data)
{
  
  iostrings *io = (iostrings * )data;
  
  
  H5O_info_t      infobuf;
  H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);

  if ( infobuf.type == H5O_TYPE_GROUP ){
    hid_t g = H5Gopen(loc_id, name, H5P_DEFAULT );
    if ( H5Lexists( g, io->first.c_str(), H5P_DEFAULT ) ) {
      herr_t err = H5Lmove( g, io->first.c_str(), g, io->second.c_str(), H5P_DEFAULT, H5P_DEFAULT );
    }
    H5Gclose(g);
  }
    
  return 0; 
}


int
main (int argc, char **argv) {
  int c;
  string from;                  // group name to replace
  string to;                    // what to replace it with
  
  string usage("  -o,--old name -n --new name  infile");

     
  while (1)
  {
    static struct option long_options[] =
      {
        /* These options don't set a flag.
           We distinguish them by their indices. */
        {"old", required_argument,       0, 'o'},
        {"new",   required_argument, 0, 'n'},
        {"help",   no_argument, 0, 'h'},
        {0, 0, 0, 0}
      };
    /* getopt_long stores the option index here. */
    int option_index = 0;

     
    c = getopt_long (argc, argv, "ho:n:", long_options, &option_index);
     
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
     
     
      case 'o':
        from = string(optarg);
        break;
      case 'n':
        to = string(optarg);
        break;
     
    case 'h':
      cout << argv[0]  << usage << endl;
      return -1;
      break;
      
      case '?':
        /* getopt_long already printed an error message. */
        break;
     
      default:
        abort ();
    }
  }
     
  
  if ( from == string() || to ==  string() ){
    cerr << argv[0]  << usage << endl;
    return -1;
  }
  
     
  string infile(argv[optind++]);
  if (optind < argc)
  {
    cerr << argv[0]  << usage << endl;
    return -1;
  }
   
  rename_groups (infile, from, to);
  
  exit (0);
}
  


