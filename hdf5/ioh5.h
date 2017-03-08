#ifndef IOH5_H
#define IOH5_H
/* 
   Copyright (c) 2013-2015 Claude Lacoursiere

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







#include <string>
#include <vector>
#include <valarray>
#include <typeinfo>
#include <string.h>
#include <hdf5.h>
#include <iostream>

#ifdef _MSC_VER
#include <cpp/H5Cpp.h>
#else
#include <H5Cpp.h>
#endif
#include <H5Tpublic.h>
#include <H5Exception.h>

/**
   The point of this library is to avoid the verbose API of HDF5 for the
   specialized case of writing vectors, matrices, scalars, and strings. 

   The verbosity of HDF5 is stunning as one has to create properties,
   dataspaces, and then datasets before writing anything to a file.
   However these can be generated automatically in most cases if one simply
   knows the atomic type of an object, as can be done with the typinfo
   library, and the size of said which, in good C++ code, is easily found
   from the container types which all have a size() method.  

   The datatype concept in HDF5 is such that it does not really allow
   templating and so we resort to dynamic typing.  But that's hiddent from
   the present API in order to have real templates, which applies
   especially to container types, as long as they are sequential.  

   The translations from standard atomic types and the HDF5 ones is done
   with the typinfo utility and a static table.  This is not really elegant
   code but it works.  In particular, this opens the possibility of reading
   an array of float from an array of doulbe stored in a file without
   anyone the wiser.  In the author's opinion, it is this type of issue
   that makes HDF5 seem unusable to most, which is regrettable. 

   What happens here is that if one says: 

   write(group, name, object);

   then this resolves to a general, raw_dump function which uses dynamic
   typing to use the basic HDF5 API. 

   object will be a container or a scalar type, or a string type. 

   Likewise if one says
   read(group, name, store);

   then the type of the objects containted in "store"  will be used to
   convert whathever is found in the "name"  dataset under the group
   "group" to what "store"  can contain, and assuming that "store" is a
   container class with a "resize" method, all memory allocation will be
   performed automatically.  
   
   With this, one can then construct HDF5 write functions which can handle
   complex objects such as sparse matrices and such.  Utilities are
   provided to store Octave sparse matrix types for instance.  

   Reading from files is also easy.  Exceptions thrown from the HDF5 C++
   API are handled more or less gracefully.  This needs work. 

   Another number of utilities are provided such as creating a path to a
   group recursively. 


*/


/** TODO:

 
    Missing in this implementation is a way to handle bitfields, and
    some better way to handle enumerations.  

    The "stride" argument used in the journal should be used to set the
    column size of the array being dumped since this is in fact valuable
    information when time comes to read back.  
    
*/


/**
   The h5 namespace provides functionality for saving/loading simple data
   data to/from HDF5 files.  Covered here are array objects of any type, as
   long as they have size() and  resize() operators, and contiguous storage
   from &v[0]
*/
namespace h5 {

  /** 
      A wrapper class for the annoying hsize_t dims[2] arrays which are
      used all the time and have to be declared and initialzed.  This here
      allows to put anonymous objects  h2s(a,b) as arguments to hdf5
      functions instead for first instantiating a 2D array, then passing it
      to the function.
  */
  struct  h2s  { 
  public: 
    hsize_t m[2];               // struct h2s is automatically cast to hsie_t [2]
    h2s(hsize_t a, hsize_t b) { m[0] =a; m[1]=b;}
    h2s(hsize_t a) { m[0] =a; m[1]=1;}
    h2s() { m[0] =1; m[1]=1;}
#if !defined(__APPLE__) 
#ifndef WIN32
    //\TODO Fix mac & windows compiling
    hsize_t & operator[](size_t i) { return m[i] ; }
    hsize_t operator[](size_t i) const  { return m[i] ; }

    size_t  get_size(const H5::DataSpace& space){

      size_t size = 1;
      
      size_t N = space.getSimpleExtentNdims();
      
      if ( N ) { 
        hsize_t  d[ N ];
        space.getSimpleExtentDims( d );
        size  = d[ 0 ];
        m[ 0 ] = d[ 0 ];
        m[ 1 ] = d[ 1 ];
        for (size_t i = 1; i < N ;  ++i ) size *= d[ i ] ; 
      }
      
      return size; 
    }

#endif
#endif
    operator const hsize_t * () const { return m ; }
    operator hsize_t * () { return m ; }
  };



  ///
  /// Type information needed by HDF5 functions collected here to allow
  /// extensive polymorphism with dynamic type identification
  ///
  ///  Predicates and types appear in predictable and nearly systematic
  ///  ways in hdf5 usage, and this allows taking care of the most common
  ///  usage i.e., writing arrays and scalars.
  ///
  ///  This is the most fundamental component of the library allowing for
  ///  dynamic typing which avoids much code duplication.  This isn't an
  ///  elegant solution but one that works. 
  ///  
  ///
  struct h5_type { 
    const std::type_info & info; 
    const H5::PredType   & predicate;
    const H5::DataType   type;
    const std::string    name;
    const hid_t         type_c;
    const unsigned char type_size;
  };


  ///
  /// Search for an h5_type according to a value returned by typeid 
  /// Support utility for the above.  Dynamic type identification from C++
  /// is used here to map to the h5_type
  ///
  const h5_type& h5_type_find(const std::type_info & type);
  ////
  /// Search for an h5_type according to a given H5 DataType.  This is most
  /// useful for reading. 
  ///
  const h5_type& h5_type_find(const H5::DataType& t );


  ///
  /// Write a string to a group
  ///
  void write_string(H5::Group &g, const std::string &name, const std::string &s);
  ///
  /// Write a UTF8 string to a group. 
  ///
  void write_string_utf8(H5::Group &g, const std::string &name, const std::string &s);
  ///
  /// Write a UTF8 string to a group, but use the hid_t from the C API
  /// instead. This is in fact called from the previous function since the
  /// C API is needed to have access to the UTF8 data. 
  ///
  void write_string_utf8(hid_t g, const std::string &name, const std::string &s);

  ///
  /// Easily create a data space for either scalars or arrays.  Exceptions
  /// are caught. 
  /// 
  H5::DataSpace  create_dataspace(const h2s& h);

  /// 
  /// H5 C++ interface does not have a usable hard link function so this
  /// one resolves to the C API via the C++ constructs.
  ///
  /// f      is the file in which we link
  /// orig   is the name of the original dataset
  /// g      is the group which will point to the other
  /// target is the name
  ///  
  herr_t hard_link(const H5::CommonFG& f, const std::string & orig, H5::Group& g, const std::string& target);

  ///
  /// Write an array with knowledge of the datatype. This is the only
  /// function containing details of the datalayout and controls the
  /// possible compression of the data on disk.  Everyting in this function
  /// uses dynamic type identification.  All 'write' functions resolve to
  /// this one which interfaces to the C API of HDF5
  ///
  H5::DataSet write_array_raw(  H5::Group & g,  const h5_type &t, 
                                const std::string& name, 
                                const void *a, size_t m=1, size_t n=1);

    
  ////
  //// This function calls the write_array_raw function after dynamic type
  /// identification which allows it to be templated.   It is called after
  /// the top API function determines the size of the array to be written. 
  /// 
  template <typename R>
  H5::DataSet write_array( H5::Group & g,  const std::string& name, const R *a, const size_t m=1, const size_t n=1){


    try {
      /// This here is a double check that the datatype type was found 
      /// The exception is thrown but write_array_raw
      const h5_type & t = h5_type_find(typeid(a[0]));
      if  ( typeid(a[0]) == t.info ) { 
        return write_array_raw(g,   t, name, a, m, n);
      } 
    }catch ( const H5::DataSetIException& exception ) {
      exception.printError();
    }
    return H5::DataSet();
  }

  ///
  /// This assumes that the datatype, such as valarray or vector, has an []
  /// operator as well as a size() method, with contiguous data from v[0].
  /// Allows for 2D array when last optional parameter is used.  Also
  /// allows writing scalar when 0 is gven as the last argument though the
  /// write_scalar function should be used to make the semantic clearer.
  ///

  template <typename R>
  H5::DataSet write_vector(  H5::Group & g,  const std::string& name, const R& v, size_t n=1){
    return write_array(g, name,  &(v[0]), v.size()/n, n);
  }

  /// 
  /// Write a scalar to the file, using the write_array function with size
  /// =1, 0.  All handling of the subtle differences between scalars and
  /// arrays are handled in the "raw"  function which can distinguish
  /// between arrays of size (1,1) and scalars. 
  ///
  template <typename R> 
  H5::DataSet write_scalar(H5::Group & g, const std::string &name, const R a){
    return write_array(g, name,  &a, 1, 0);
  }

  
  ///
  ///  Write a scalar attribute of any type except for string.  
  ///
  template <typename R> 
  void set_scalar_attribute (H5::H5Object &g, const std::string &name, const R& val){
    try {
      const h5_type &  t = h5_type_find(typeid(val));
      if  ( typeid(val) == t.info ) { 
        H5::Attribute attr = g.createAttribute(name, t.type, H5::DataSpace(H5S_SCALAR));
        attr.write(t.type, &val);
      } else {
      }
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

  /// Strings have to be handled separately
  template<>
  void set_scalar_attribute(H5::H5Object& g, const std::string &name, const std::string& val);
 
  
  ///
  /// Utility
  ///
  void print_attributes( H5::H5Object& loc, const H5std_string attr_name, void *operator_data); 

  /**
     Read the value of the attribute with the given name from the given group.
     \return True if a write was made to val, false if there is no such attribute,
     or if the attribute was of the wrong type. 
     \todo this should be rearranged to be type agnostic. 
     \todo as of Tue Jun 16 21:04:08 CEST 2015 I have no idea what I meant
     in the comment above (CL)
  */
  template < typename T >
  bool read_scalar_attribute(H5::H5Object & g, const std::string& name, T& result){
    try {
      const h5_type & t = h5_type_find(typeid(result));
      H5::Attribute attribute; 
      try{ 
        attribute = g.openAttribute(name);
      } catch ( const H5::AttributeIException &	 exception ) {
        return false;
      }
      try {
        if ( attribute.getDataType() == t.predicate ) {
          attribute.read( attribute.getDataType(), &result );
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

  /**
     Read the value of the attribute with the given name from the given group.
     \return True if a write was made to val, false if there is no such attribute,
     or if the attribute was of the wrong type.   This is specialized to
     strings because special handling is needed.
  */
  template <>
  bool read_scalar_attribute<std::string>(H5::H5Object & g, const std::string& name, std::string & result);
 
  /** 
      Open and existing file to append a problem set, or create a 
      new one.  This does not overwrite existing data. 
        
  */

  H5::H5File * append_or_create(const std::string & filename);

  /** 
   * Create a new group to write a dataset.  

   \todo No utility is provided here  to delete problems.  

  */

  H5::Group  append_problem(H5::H5File *file, const std::string & name = "Problem", int width  = 6);
  H5::Group  append_problem(H5::Group &group, const std::string & name = "Problem", int width = 6);

  /**
   *  Appends the given problem and don't number it. 
   **/
  H5::Group  append(H5::Group &group, const std::string & name = "Problem", int width = 6);

  /**
     \return The index of the child with the given name. Returns parent.getNumObjs() if no such child exists.
  */
  hsize_t getIndexOf( const H5::CommonFG& parent, const H5std_string& childName );

  /** 
      Find a child with given name.
  */

  template < class T > 
  bool hasChildNamed( const T& parent, const std::string& childName ){
    return H5Lexists( parent.getLocId(), childName.c_str(), H5P_DEFAULT ) > 0;
  }




  /**
     Return the child group with the given name. Will be created if it doesn't exist.

     This is the same principle as append_or_create.  This library is
     intended only for writing datasets, not to manipulate them. 
  */
  H5::Group getOrCreateGroup( H5::CommonFG& parent, const H5std_string& childName );



  /**  A utility that just checks for the existence of an attribute with given name */
  bool check_attr(const H5::CommonFG &g, const std::string & name);


  /** Read the string dataset of given name into the std::string buff.
      Return 0 if not found, 1 for success.  */
  int read_string(const H5::CommonFG& g, const std::string &name, std::string & buff );
  int read_string(hid_t g, const std::string &name, std::string & buff );


  /** 
      Read an array and put content in object v which must be a class with a method 
      resize(size_t n) and operator *  which returns the data buffer. 
  */ 
  template<typename R>
  H5::DataType read_array(const H5::Group& g, const std::string &name, R& v )
  {
    try { 
      H5::Exception::dontPrint();
      H5::DataSet t  = g.openDataSet(name);
      try { 
        H5::Exception::dontPrint();
        H5::DataSpace ds = t.getSpace(); 
        // this is to make sure that we have enough room
        const h5_type & type = h5::h5_type_find(typeid(v[0]));
        size_t  N = h2s().get_size(ds);
        // make sure we have enough space
        size_t s1 = sizeof(v[0]); 
        size_t s2 = t.getDataType().getSize(); 
        size_t alloc = N; 
        if ( s2 > s1 ) alloc *= s2/s1; 
        R  w(alloc); 
        t.read(&(w[0]), t.getDataType() );
        // at this point, we can't be certain that we have the correct
        // datatype for the array that was given so we use the conversion
        // if necessary. 
        t.getDataType().convert(type.type, N , &(w[0]), NULL, H5::PropList());
        t.close(); 
        v.resize(N); 
        memcpy(&(v[0]), &(w[0]), N * sizeof(v[0]));
        return  type.type;
      } catch ( H5::DataSetIException& error ) { 
      } 
    }
    catch (H5::GroupIException& error ) { 
    }
    return H5::DataType();
  }

  template<typename R>
  H5::DataType read_vector(const H5::Group& g, const std::string &name, R& v ){
    return read_array(g, name, v ) ; 
  }

  template <typename R>
  bool read_scalar(H5::Group &g, const std::string& name, R& result)
  {
    const h5_type &  t = h5_type_find(typeid(result));
    const h5_type & type = h5::h5_type_find(typeid(result));
    try {
      H5::DataSet dataset = g.openDataSet(name);
      if  ( dataset.getSpace().getSimpleExtentType() == H5S_SCALAR ) {
        if ( true || dataset.getDataType() == t.predicate  ) { // need error message or exception if this is false
          try {
            char raw[128];      // to hold raw bytes
            dataset.read((void *)raw,  dataset.getDataType() );
            dataset.getDataType().convert(type.type, 1 , (void *)raw, NULL, H5::PropList());
            memcpy(&result,(void *)raw, sizeof(result));
            return true;
          } catch ( const H5::DataSetIException& exception ) {
            return false;
          } 
        } else { 
        }
      } else {
        std::vector<R> tmp(1);  // try to read this as a 1 x 1 array
        read_array(g, name, tmp);
        result = tmp[0];
        return false;
      }
    } catch ( const H5::DataSetIException& exception ) {
      return false;
    } 
    return false;
  }
   
//
// This version is intendent for the case where we don't know the datatype
// at all and are only interested in working on the raw storage as char
// The template remains since we might have valarray<char>  or
// vector<char> for instance, i.e., the template type must be a container
// of char.  However, not knowing the data type, we
// cannot use the resize operator using the information from the
// dataspace.   The purpose is to be able to apply filters without first
// making a determination of the datatype and then instantiation of arrays
// with the correct version. 
// 
// "Raw"  here means that we have just bytes to manipulate. 
//
//  Templating to CommonFG instead of Group proved impossible
// 
  template<typename R>
  H5::DataType   read_array_raw(const H5::Group& g, const std::string &name, R& v)
  {
    H5::DataType ret;
    try { 
      H5::Exception::dontPrint();
      if ( g.getNumObjs()) { 
        H5::DataSet t  = g.openDataSet(name);
        H5::DataSpace ds = t.getSpace(); 
        h5::h2s dims; 
        v.resize( dims.get_size( t.getSpace() )  * t.getDataType().getSize()  );
        
        if ( v.size() ) {
          t.read(&(v[0]), t.getDataType(), ds);
        }
        ret = t.getDataType(); 
        ds.close();
        
        t.close(); 
        return ret;
      }
    }
    catch ( H5::DataSetIException& error ) { 
    } 
    catch (H5::GroupIException& error ) { 
    }
    return H5::DataType();
  }

  ///
  /// Takes a string and remove separating slashes
  ///
  std::string remove_slash(const std::string s);
  
  ///
  ///  Create the full path to a group as needed.
  ///
  H5::Group create_path(const std::string& names, H5::Group &g) ;
  

///////////////////////////////////////////////////////
///
/// Utilities to locate datasets which have nown names but unknown location
/// in the file. 
///
///////////////////////////////////////////////////////


/// 
/// This allows operating on the idx' child of g with an iterator 
/// 
  struct TraverserFn { 
    virtual bool operator()(H5::Group& parent, hsize_t idx) = 0;
  };


/// 
///  A traverser function which matches a name during a depth first
///  traversal from a given group.
  /// \todo  This is probably wrong since the hasChildNamed function can be
  /// used instead and that is then much faster.  This means that the entire
  /// traverser has to be revised. 
///

  struct MatchName : public TraverserFn {
    H5std_string target_name; 
    hsize_t child_index;
    H5::Group parent;
    bool got_match;
    
    bool operator()(H5::Group& g, hsize_t idx){

      if ( g.getObjnameByIdx(idx) == target_name ) {
        child_index = idx;
        parent = g;
        return true;
      } 
      return false;
    }

    MatchName(const std::string& s) : target_name(s),  child_index(-1), parent(), got_match(false) {};


    void set_name(const std::string& s) { 
      target_name = s;  
      child_index = -1;
      parent = H5::Group();
      return;
    }

    ~MatchName() { }

  };

  
////
/// This is a depth first search operation which starts from a group.
/// The "fn"  argument matches the sought conditions. 
/// 
/// This could be designed to start from a file but this causes problems to
/// return the correct object type in the MatchName struct for instance. 
/// One can always open the "/" group in a file and start from there. 
///
  bool  find_first(H5::Group g, TraverserFn& fn); 

  const std::string oct_format = "OCTAVE_NEW_FORMAT";
  const std::string oct_type = "type";
  const std::string oct_sparse_matrix = "sparse matrix";
  const std::string oct_sparse_row_index = "ridx";
  const std::string oct_sparse_col_pointers = "cidx";
  const std::string oct_sparse_data = "data";
  const std::string oct_sparse_rows = "nr";
  const std::string oct_sparse_cols = "nc";
  const std::string oct_sparse_nnz = "nz";
  const std::string oct_matrix = "matrix";
  const std::string oct_scalar = "scalar";
  const std::string oct_value = "value";

  ///
  ///  
  ///  Fetch a sparse matrix written by octave.  This is templated to avoid
  ///  problems with int and size_t, as well as getting data in std::vector
  ///  or std::valarray or something else.  The distinction between the
  ///  ptr and idx vector is a bit far fetched but it cannot hurt. 
  ///
  template <class U, class V, class R, class S, class T> 
  bool read_sparse_octave(H5::CommonFG& f, 
                          const std::string name, 
                          U&  m, V&  n, R& ptr, S& idx, T& data){
    bool ret = false;
    if ( h5::hasChildNamed(f, name ) ) { 
      H5::Group g = f.openGroup(name);
      /// check that it has a dataset named "type"
      if ( h5::hasChildNamed(g, oct_type) ) { 
        std::string type_name;
        read_string(g, oct_type, type_name);
        if ( type_name == oct_sparse_matrix ){
          H5::Group values = g.openGroup(oct_value);
          read_array(values, oct_sparse_col_pointers, ptr);
          read_array(values, oct_sparse_row_index, idx);
          read_array(values, oct_sparse_data, data);
          read_scalar(values, oct_sparse_cols, m);
          read_scalar(values, oct_sparse_rows, n);
          ret = true;
        }
      }
    }
    return ret; 
  }


  template <class R, class I> 
  void write_sparse_octave(H5::CommonFG& g, const std::string name, int m,
                           int n, const I& ptr, 
                           const I& idx, const R& data){
    H5::Group gg = g.createGroup(name); // matrix name
    set_scalar_attribute(gg, oct_format, 1);
    write_string(gg, oct_type, oct_sparse_matrix);
    H5::Group gh = gg.createGroup("value"); 
    write_vector(   gh,   "data", data);
    write_vector(   gh,   "cidx", ptr);
    write_vector(   gh,   "ridx", idx);
    write_scalar(   gh,   "nr", m);
    write_scalar(   gh,   "nc", n);
    write_scalar(   gh,   "nz", data.size());
  }




  ///
  /// HDF5 has an outrageously complicated way of fetching info from a
  /// dataset so this collects the simple information usually required
  ///
  struct h5_dataset_info{
    const size_t  n_points;           // total number of entries in dataset
    const size_t storage_size;        // total amount of space occupied
    const h5_type  &type;              // type information
    std::vector<hsize_t> dims;   // dimension sizes: dims.size() is the
    // number of dimensions
    h5_dataset_info( const H5::DataSet & ds): 
      n_points( ds.getSpace().getSimpleExtentNpoints() ), 
      storage_size( ds.getStorageSize() ), 
      dims( ds.getSpace().getSimpleExtentNdims() ),
      type(h5_type_find( ds.getDataType() ) ) 
    {
      ds.getSpace().getSimpleExtentDims( &(dims[0]) );
    }
                                         
  private:
    h5_dataset_info();
    
  };

  void print_type_list ( );
  

}



#endif


