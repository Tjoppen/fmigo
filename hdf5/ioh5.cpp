/* 
   Copyright (c) 2013 Claude Lacoursiere

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

*/ 



#include "ioh5.h"
#include <sstream>              // for string manipulations.
#include <iomanip>
#include <iostream>
#include <string>
#include <typeinfo>
#include <stdint.h>
#include <boost/regex.hpp>
#include <utility>


using namespace std;
using namespace boost;

using namespace H5;
namespace h5 { 



  /// Verify that a given attribute is in the given group
  bool check_attr(const H5::H5Object &g, const std::string & name){
    bool ret = false; 
    try { 
      Exception::dontPrint();
      Attribute a = g.openAttribute(name);
      a.close();
      ret = true;
    }
    catch(H5::AttributeIException& error) { 
    }
    return ret;
  }


// Below is the complete list of atomic datatypes provided by HDF5.  The
// logic here is to do runtime type identification so we have one and only
// one templated function to write scalars, one and only one to write
// one or two dimensional arrays.   The list is static and only used for
// lookup.  This type of clumsy initialization is a necessary evil. 
  static    const h5_type h5_type_list[] = {
    {typeid(char)                  , PredType::NATIVE_CHAR,    IntType   ( PredType::NATIVE_CHAR    ),   "char"                   ,H5T_NATIVE_CHAR     , sizeof(char) },
    {typeid(signed char)           , PredType::NATIVE_SCHAR,   IntType   ( PredType::NATIVE_SCHAR   ),   "signed char"            ,H5T_NATIVE_SCHAR  , sizeof(signed char)   },
    {typeid(unsigned char)         , PredType::NATIVE_UCHAR,   IntType   ( PredType::NATIVE_UCHAR   ),   "unsighed char"          ,H5T_NATIVE_UCHAR  , sizeof(unsigned char)   },
    {typeid(short int)             , PredType::NATIVE_SHORT,   IntType   ( PredType::NATIVE_SHORT   ),   "short int"              ,H5T_NATIVE_SHORT     , sizeof(short int)},
    {typeid(unsigned short int)    , PredType::NATIVE_USHORT,  IntType   ( PredType::NATIVE_USHORT  ),   "unsigned short int"     ,H5T_NATIVE_USHORT    , sizeof(unsigned short int )},
    {typeid(int)                   , PredType::NATIVE_INT,     IntType   ( PredType::NATIVE_INT     ),   "int"                    ,H5T_NATIVE_INT      , sizeof(int)},
    {typeid(unsigned int)          , PredType::NATIVE_UINT,    IntType   ( PredType::NATIVE_UINT    ),   "unsigned int"           ,H5T_NATIVE_UINT    ,  sizeof(unsigned int)  },
    {typeid(long int)              , PredType::NATIVE_LONG,    IntType   ( PredType::NATIVE_LONG    ),   "long int"               ,H5T_NATIVE_LONG   , sizeof(long int)   },
    {typeid(unsigned long int)     , PredType::NATIVE_ULONG,   IntType   ( PredType::NATIVE_ULONG   ),   "unsigned long int"      ,H5T_NATIVE_ULONG   ,sizeof(unsigned long int)   },
    {typeid(long long int)         , PredType::NATIVE_LLONG,   IntType   ( PredType::NATIVE_LLONG   ),   "long long int"          ,H5T_NATIVE_LLONG   , sizeof(long long int)  },
    {typeid(unsigned long long int), PredType::NATIVE_ULLONG,  IntType   ( PredType::NATIVE_ULLONG  ),   "unsigned long long int" ,H5T_NATIVE_ULLONG    , sizeof(unsigned long long int)},
    {typeid(float)                 , PredType::NATIVE_FLOAT,   FloatType ( PredType::NATIVE_FLOAT   ),   "float"                  ,H5T_NATIVE_FLOAT     , sizeof(float)},
    {typeid(double)                , PredType::NATIVE_DOUBLE,  FloatType ( PredType::NATIVE_DOUBLE  ),   "double"                 ,H5T_NATIVE_DOUBLE   , sizeof(double)},
    {typeid(long double)           , PredType::NATIVE_LDOUBLE, FloatType ( PredType::NATIVE_LDOUBLE ),   "long double"            ,H5T_NATIVE_LDOUBLE   , sizeof(long double)},
/*  These are bit field types which are not needed yet but left here for completeness
    PredType::NATIVE_B8
    PredType::NATIVE_B16
    PredType::NATIVE_B32
    PredType::NATIVE_B64
*/
/* These are mostly used by HDF5 only.  Again, this is here so that this list is as close as possible to the list of types provided by HDF5
   PredType::NATIVE_OPAQUE
   PredType::NATIVE_HSIZE
   PredType::NATIVE_HSSIZE
   PredType::NATIVE_HERR
*/ 
    {typeid(bool)                  , PredType::NATIVE_HBOOL,   IntType  (PredType::NATIVE_HBOOL     ),   "bool"                    ,H5T_NATIVE_HBOOL     ,  sizeof(bool)},
    {typeid(int8_t)                , PredType::NATIVE_INT8,    IntType  (PredType::NATIVE_INT8      ),   "int 8"                          ,H5T_NATIVE_INT8     , sizeof(bool) },
    {typeid(uint8_t)               , PredType::NATIVE_UINT8,   IntType  (PredType::NATIVE_UINT8     ),   "uint 8"                 ,H5T_NATIVE_UINT8     , sizeof(uint8_t)},
    {typeid(int16_t)               , PredType::NATIVE_INT16,   IntType  (PredType::NATIVE_INT16     ),   "int 16"                 ,H5T_NATIVE_INT16     , sizeof(int16_t)},
    {typeid(uint16_t)              , PredType::NATIVE_UINT16,  IntType  (PredType::NATIVE_UINT16    ),   "uint 16"                ,H5T_NATIVE_UINT16    , sizeof(uint16_t)},
    {typeid(int32_t)               , PredType::NATIVE_INT32,   IntType  (PredType::NATIVE_INT32     ),   "int32"                  ,H5T_NATIVE_INT32     , sizeof(int32_t)},
    {typeid(uint32_t)              , PredType::NATIVE_UINT32,  IntType  (PredType::NATIVE_UINT32    ),   "uint32"                 ,H5T_NATIVE_UINT32    , sizeof(uint32_t)},
    {typeid(int64_t)               , PredType::NATIVE_INT64,   IntType  (PredType::NATIVE_INT64     ),   "int64"                  ,H5T_NATIVE_INT64     , sizeof(int64_t)},
    {typeid(uint64_t)              , PredType::NATIVE_UINT64,  IntType  (PredType::NATIVE_UINT64    ),   "uint64"                 ,H5T_NATIVE_UINT64    , sizeof(uint64_t)},
    {typeid(std::string)           , PredType::C_S1         ,  StrType  (                           ),   "string"                 ,H5T_C_S1             , sizeof(std::string)},
    {typeid(H5std_string)          , PredType::C_S1         ,  StrType  (                           ),   "string"                 ,H5T_C_S1            , sizeof(H5std_string)},
    /** though this would be nice, plain old char * strings don't quite behave so this is ignored
        {typeid(char*), PredType::C_S1, StrType()},

        Last entry is garbage indicating that we have not found the datatype. 
        That's detected at runtime by checking the typeinfo on the variable
        you are trying to save.  New entries must be entred above this one. 
    */
    {typeid(void*)               , PredType::NATIVE_UINT64,  IntType(PredType::NATIVE_UINT64      ),   "UNKNOWN", (const unsigned char)-1, (const unsigned char)-1}
    // I mean it, DON'T put anything below this. 
  };







  // 
  // Unify creation of scalar and array data spaces.  
  // 
  H5::DataSpace   create_dataspace(const h2s & h){
    H5::DataSpace ret;
    try {
      if ( h[0] ==1 && h[1] == 1){
        ret = H5::DataSpace(H5S_SCALAR);
      }
      else{
        /// should use operator overloading to skip this
        const hsize_t *hh  = h; 
        ret = H5::DataSpace(2, hh);
      }
    } catch ( H5::DataSpaceIException& error  ){
    }
    return ret;
  }

  // 
  // Type identification to simplify the construction of dataypes. 
  // This return the correct entry if found or garbage otherwise.  If the type
  // is not found, the error flag in the returned struct 
  // This is a linear search. A hash table might be more efficient. 
  const h5_type& h5_type_find(const std::type_info& type) {
// Ignore the last entry for the search
    size_t n_items = sizeof(h5_type_list)/sizeof(h5_type_list[0]) -1 ; 
    size_t i=0;

    for (i=0; i < n_items; ++i )
      if ( h5_type_list[i].info == type )  break;

    // the following is safe since if we reached i == n_items, we are
    // returning the garbage entry which will not match the type of the
    // argument. 
    // Error checking is left to the caller. 
    return h5_type_list[i];
  }
 
  const h5_type&  h5_type_find(const H5::DataType& type) {
    size_t n_items = sizeof(h5_type_list)/sizeof(h5_type_list[0]) -1 ; 
    size_t i=0;
    for (i=0; i < n_items; ++i )
      if ( h5_type_list[i].type == type )  break;
    // Error checking is left to the caller. 
    return h5_type_list[i];
  }   


  // Read a string with attribute "name" from the named group or file in the buff argument
  int read_string(const H5::CommonFG& g, const std::string &name, std::string & buff  ) { 
    int ret = 0; 
    try { 
      Exception::dontPrint();
      DataSet t  = g.openDataSet(name);
      try { 
        std::string tmp;
        tmp.resize(t.getStorageSize());
        t.read(tmp, t.getStrType());
        buff = tmp;
        t.close(); 
        ret = 1; 
      } catch ( H5::DataSetIException& error ) { 
      } 
    }
    catch (H5::GroupIException& error ) { 
    }
    return ret;
  }
    
  // return an integer named "name" from a group or file
  int read_integer(const H5::CommonFG& g, const std::string &name  ) { 
    int ret = 0; 
    try { 
      Exception::dontPrint();
      DataSet t  = g.openDataSet(name);
      try { 
        Exception::dontPrint();
        t.read(&ret, t.getIntType());
        t.close(); 
      } catch ( H5::DataSetIException& error ) { 
      } 
    }
    catch (H5::GroupIException& error ) { 
    }
    return ret;
  }
    
  /// 
  /// Create a hard link so that group or dataset named orig under source
  /// group f  is linked to group or dataset target unger group g
  /// 
  /// Curiously, the C++ API does not provide this simple and necessary
  /// operation, and no explanation is provided as to why. 
  ///
  herr_t hard_link(const H5::CommonFG& f, const std::string & orig, H5::Group& g, const std::string& target){
    return H5Lcreate_hard( f.getLocId(), orig.c_str(), g.getLocId(), target.c_str(), 0, 0);
  }
    
  ///
  /// The name says it all.  This works for files or groups. 
  ///
  /// Revert to the C API because this utf8 functionality is missing from
  /// the C++
  /// 
  ///
  void write_string_utf8(hid_t gid, const std::string &name, const std::string &s){

    /// this here I have no idea what that's for.  It is for variable
    /// memory size yes, but without this and the trick below to put the
    /// string into an array, it is impossible to write to a file properly
    /// with the C API.  The C++ version is simpler but then, there doesn't
    /// seem to be a way to achieve the goal. 
    hid_t memtype = H5Tcopy (H5T_C_S1);
    H5Tset_size (memtype, H5T_VARIABLE);

    /// Sice we declared variable size, we can set the sizes to default
    hid_t dataspace = H5Screate_simple (1, h2s(), NULL);
    hid_t utf8_predicate = H5Pcreate( H5P_LINK_CREATE );
    H5Pset_char_encoding(utf8_predicate, H5T_CSET_UTF8) ;
    /// 
    hid_t dataset = H5Dcreate2 (gid, name.c_str(), memtype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    /// This trick wraps the cstring so that it is accessed via a pointer
    const char * c[] = {s.c_str()};
    H5Dwrite (dataset, memtype,  H5S_ALL, H5S_ALL, H5P_DEFAULT, c);
    H5Dclose (dataset);
  }
  void write_string_utf8(H5::Group& g, const std::string &name, const std::string &s){
    write_string_utf8(g.getLocId(),  name, s);
  }
  

  // The name says it all.  This works for files or groups
  void write_string(H5::Group&g, const std::string &name, const std::string &s){
    try {
      H5::DataSpace dataspace( 0, h2s());
      StrType datatype(PredType::C_S1, s.length()+1);
      H5::DataSet dataset = g.createDataSet( name, datatype, dataspace );
      dataset.write( s, datatype);
      dataset.close();
    } catch ( const H5::DataSetIException& exception ) {
      exception.printError();
    }
  }



  /** 
      Open an existing H5File to append or create it from scratch if it
      does not exist.  It is the caller's responsibility to close that file
      when writing is done. 

      The logic here is that datasets are valuable and one should not 
      just truncate an existing file.  This library is designed so there 
      is no possibility to overwrite data committed to a file by accident.  
  */
  H5::H5File * append_or_create(const std::string & filename) { 

    H5File *file = 0; 
    try { 
      Exception::dontPrint();
      file = new H5File(filename, H5F_ACC_RDWR);
    } catch( FileIException& error ) { 
      try {
        file = new H5File(filename, H5F_ACC_TRUNC);
      } catch (FileIException& foo){
      }
    }
    return file; 
  }
    
  /** Open a group with a unique name in an existing H5File.  The groups
      are labelled sequentially with an integer.  

      The H5::Group destructor closes the group automatically.  However, if a
      groupis reused, it must be closed by the caller. 

      This function will not ovewrite data but append at the end of 
      the list of groups in a file. 
  */
  H5::Group  append_problem(H5File *file, const std::string & name, int width ) { 
    H5::Group  root = file->openGroup("/");
    return append_problem(root, name, width);
  }

  H5::Group  append_problem(Group & g,  const std::string & name, int width ) { 
    int n_groups = g.getNumObjs();
    std::ostringstream buffer; 
    buffer << name << std::setw(width) << std::setfill('0') << ++n_groups; 
    std::string w = buffer.str();
    return g.createGroup(buffer.str());
  }

  H5::Group  append(Group & g,  const std::string & name, int width ) { 
    std::ostringstream buffer; 
    buffer << name; 
    std::string w = buffer.str();
    return g.createGroup(buffer.str());
  }


  hsize_t getIndexOf( const H5::CommonFG* parent, const H5std_string& childName )
  {
    /// \todo This cannot be the proper way to find the index of a named child.
    /// NOTE: the correct way is to use an iterator function
    hsize_t numObjects = parent->getNumObjs();
    for ( size_t i = 0 ; i < numObjects ; ++i )
    {
      H5std_string name = parent->getObjnameByIdx( i );
      if ( name == childName )
        return i;
    }
    return numObjects;
  }

  H5::Group getOrCreateGroup(H5::CommonFG& parent, const H5std_string& childName )
  {
    if ( hasChildNamed(parent, childName) ) {
      
      return parent.openGroup( childName ) ;
    }
    return parent.createGroup( childName );
  }
  void print_attributes( H5::H5Object& loc, const H5std_string attr_name, void *operator_data) {
    // this to make gcc shut up about unused arguments
    H5::H5Object *l = &loc; 
    void * f = operator_data; 
    f = (void *)l;  
    l  = (H5::H5Object *)f; 

    // would be nice to find the name of the parent.  Dunno yet. 
    //std::cerr << "Found attribute : " << attr_name << std::endl;
  }


  //
  //  Specialization for strings. 
  // 
  template<>
  void set_scalar_attribute(H5::H5Object& g, const std::string &name, const std::string& val){
    try {
      H5::StrType datatype(H5::PredType::C_S1,  val.size()+1 );
      H5::Attribute attr = g.createAttribute(name, datatype, H5::DataSpace(H5S_SCALAR));
      attr.write(datatype, val);
    }
    catch ( const H5::AttributeIException& exception ) {
      exception.printError();
      return;
    }
    catch ( const H5::DataTypeIException& exception ) {
      exception.printError();
      return;
    }
  }
    


  /**
     Template specialization for strings. 
       
     Read the value of the attribute with the given name from the given group.
     \return True if a write was made to val, false if there is no such attribute,
     or if the attribute was of the wrong type. 

  */
  template <>
  bool read_scalar_attribute(H5::H5Object & g, const std::string& name, std::string & result){
    try {
      H5::Attribute attribute; 
      try{ 
        attribute = g.openAttribute(name);
      } catch ( const H5::AttributeIException &	 exception ) {
        return false;
      }
      try {
        if ( attribute.getTypeClass() == H5T_STRING){
          attribute.read( attribute.getDataType(), result );
          return true;
        } 
      } catch ( const H5::DataTypeIException& exception ) {
        return false;
      }
    } catch ( const H5::DataTypeIException& exception ) {
      return false;
    }
    return false;
  }

  ///
  /// Dump an array with knowledge of the datatype. This is the only
  /// function containing details of the datalayout and controls the
  /// possible compression of the data on disk. 
  H5::DataSet write_array_raw(  H5::Group & g,  const h5_type& t, 
                                const std::string& name, const void *a, const size_t m, const size_t n){

    if ( m == 0 ) {
      return H5::DataSet() ; 
    }
    

    try{
      H5::DataSetIException::dontPrint();
      H5::DSetCreatPropList prop; 

#ifdef H5_LIBRARY_IS_FUNCTIONAL

      H5::DataSet dataset = g.createDataSet( name, t.type, create_dataspace(h2s(m, n)), prop);
      dataset.write( a, t.predicate);

#else   // fallback to the C API
      {
        hid_t file = g.getId();
        if ( H5Iis_valid( file ) <= 0 ){
          std::cerr << " bad group id  " << std::endl;
        }
        hid_t       datatype, dataspace, dataset, type_id, plist;   /* handles for dump*/
        int rank = 2; 
        hsize_t dims[] = {(hsize_t)m, (hsize_t)n}; 
  
        type_id = t.type_c; 
        datatype  = H5Tcopy(type_id);
        plist     = H5Pcreate(H5P_DATASET_CREATE);
        if ( n == 0) { 
          rank = 1;
          dataspace = H5Screate(H5S_SCALAR);
        }
        else{
          dataspace = H5Screate_simple(rank, dims, dims); 
#if 0 
          if ( dataspace < 0 ){
          std::cerr << " can't create dataspace " << std::endl;
        }
#endif
          H5Pset_chunk(plist, rank, dims);
        }
        dataset = H5Dcreate2(file, name.c_str(),  datatype, dataspace, H5P_DEFAULT, plist, H5P_DEFAULT);
#if 0 
          if ( dataset < 0 ){
          std::cerr << " can't create dataset " << std::endl;
        }
#endif
        herr_t err = H5Dwrite(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, a);
#if 0 
        if ( err < 0 ){
          std::cerr << " fuckup!  " << std::endl;
        } else {
          std::cerr << " Sucessful write? " << endl;
          
        }
#endif
        /* Close/release resources. */
        H5Dclose(dataset);
        H5Sclose(dataspace);
        H5Tclose(datatype);
        H5Pclose(plist);
      }

#endif
      return H5::DataSet();
  
    }catch ( const H5::DataSetIException& exception ) {
    } 
    return H5::DataSet();
  
  }

  ///
  /// \todo NOT documented!!!
  ///

  bool  find_first(Group g, TraverserFn& fn) {

    for(size_t i=0; i<g.getNumObjs(); ++i ){

      if ( fn(g , i) ) {
        return true;
      }

      else if ( g.getObjTypeByIdx( i ) == H5G_GROUP ) {
        Group child = g.openGroup(g.getObjnameByIdx(i));
        if ( find_first(child, fn) ){
          return true; 
        }
      }
    }
    return false;
  }


///
/// utilities to remove '/' and create a path in the file
///
std::string remove_slash(const std::string s)
{
  const boost::regex e("\\A([^/]+)/?\\z");
  const std::string sep("\\1");
  return regex_replace(s, e, sep, boost::match_default | boost::format_sed);
}

typedef std::map<std::string, int, std::less<std::string> > map_type; 
  Group create_path(const std::string& names, Group &g) 
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

  void print_type_list ( ) { 
    size_t size = sizeof( h5_type_list ) / sizeof( h5_type_list[ 0 ] ) ;
    
    for ( size_t i = 0; i < size; ++i ) { 
      std::cerr <<   h5_type_list[ i ].name <<  "   " <<    h5_type_list[ i ].type_c << std::endl;
    }
  
  }
  

}  /* namespace H5 */


