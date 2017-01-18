/**
 * Author: Jonas Sandqvist
 * Date: 18-01-2017
 */
#include<utility>
#include<vector>
#include<iostream>
#include <type_traits>

using namespace std;
/// Global data for FMUs as well as maps to fetch and read from the FMI/X
/// support.
///
/// Keep two state vectors so we can backup.
/// work with references so that we can go from backup to current without
/// copy.
namespace fmi_on_storage{
typedef pair< size_t, size_t>  Bounds;
typedef vector<double> DataType;
typedef pair< DataType, DataType>  Data;
typedef vector<Bounds> DataBounds;
class FmuData {

    /** CREATE_DATA
     *  defines all common functions for storage types needed to
     *  get, set and restore data needed by the fmu
     */
#define CREATE_DATA(name)                                               \
    Data m_##name;   /* to store the data */                            \
public: inline DataType & get_current_##name() { return m_##name.first ;} \
    inline DataType & get_backup_##name()  { return m_##name.second;}   \
    /* as needed by integrator */                                       \
public: inline double * get_current_##name##_p() { return m_##name.first.data() ;} \
    inline double * get_backup_##name##_p()  { return m_##name.second.data();}

    CREATE_DATA(states);        // maybe just use one?? and use one class for each instead
    CREATE_DATA(indicators);
    CREATE_DATA(derivatives);

    DataBounds m_bounds;
    inline DataBounds & get_bounds() { return m_bounds;}
    inline size_t get_offset(const size_t s){return get_bounds().at(s).first;}
    inline size_t get_end(const size_t s){return get_bounds().at(s).second;}
    inline size_t get_size(const size_t s) {
    return get_bounds().at(s).second - get_bounds().at(s).first;
    }

public: void allocate_storage(const vector<size_t> &number_of_states)
    {
        size_t total_number_of_variables = 0;
        size_t first = 0;
        get_bounds().resize(0);
        for(auto n:number_of_states){
            total_number_of_variables += n;
            get_bounds().push_back(Bounds(first, first + n));
            first += n;
        }

        get_current_states().resize(total_number_of_variables,1);

        //allocate memory for storage of fmu_data
        get_backup_states()              =
          get_current_indicators()       =
            get_backup_indicators()      =
              get_current_derivatives()  =
                get_backup_derivatives() =  get_current_states();


    }
public: void print(DataType & current)
    {
        for(auto x: get_bounds()) {
                for(size_t i = x.first; i != x.second; ++i)
                    fprintf(stdout,"%f ",current.at(i));
                fprintf(stdout,"\n");
            }
        fprintf(stdout,"\n");
    }

    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param current_data_set Is one of get_current_{states,indicators,derivatives}()
     *  @param vec A vector with data
     */
public: void push_to(size_t client_id, DataType &current_data_set, const DataType &vec)
    {
        if(get_size(client_id) != vec.size()){
            fprintf(stderr,"error:FmuData-push_to(): client_id != vector.size!!\n");
            exit(1);
        }
        copy(vec.begin(),
             vec.end(),
             current_data_set.begin() + get_offset(client_id));
    }

    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param current_data_set Is one of get_current_{states,indicators,derivatives}()
     *  @param array An array with data
     */
public: void push_to(size_t client_id, DataType &current, double* array)
    {
        for(size_t i = get_offset(client_id); i < get_end(client_id); i++)
                current.at(i) = *array++;
    }

    /** cycle(): maybe reset?
     *  Toggles the working dataset with a backup copy from last call of sync()
     *  swap only makes shallow copies
     */
public: void cycle()
    {
        swap(m_states.first, m_states.second );
        swap(m_indicators.first, m_indicators.second );
        swap(m_derivatives.first, m_derivatives.second );
    }

    /** sync():
     *  Makes a deep copy of the current dataset to a backup dataset
     */
public: inline void sync(){
        copy(m_states.first.begin(), m_states.first.end(), m_states.second.begin() );
        copy(m_indicators.first.begin(), m_indicators.first.end(), m_indicators.second.begin() );
        copy(m_derivatives.first.begin(), m_derivatives.first.end(), m_derivatives.second.begin() );
    }

public: void test_functions(void)
    {

        fprintf(stdout,"Test of fmi_on_storage start\n");
        vector<size_t> n_variables({1,2,3});
        DataType a,b;
        allocate_storage(n_variables);
        push_to(0,get_current_states(),DataType({2}));
        push_to(2,get_current_states(),DataType({2,3,5}));
        a = get_current_states();
        b = get_backup_states();
        if(a == b) fprintf(stderr,"Error: fmi_on_storage push_to - did not push\n");
        else fprintf(stdout,"push_to past test\n");

        cycle();
        b = get_backup_states();
        if(a != b) fprintf(stderr,"Error: fmi_on_storage cycle - did not cycle\n");
        else fprintf(stdout,"cycle past test\n");
        cycle();

        sync();
        a = get_current_states();
        b = get_backup_states();
        if(a != b) fprintf(stderr,"Error: fmi_on_storage sync - did not sync\n");
        else fprintf(stdout,"sync past test\n");

        push_to(0,get_backup_states(),DataType({5}));
        push_to(1,get_backup_states(),DataType({5,351}));
        push_to(2,get_backup_states(),DataType({5,684,5135}));
        double x[1] = {5};
        double y[2] = {5,351};
        double z[3] = {5,684,5135};

        push_to(0,get_current_states(),x);
        push_to(1,get_current_states(),y);
        push_to(2,get_current_states(),z);
        a = get_backup_states();
        b = get_current_states();
        if(a != b) {
            print(get_current_states());
            print(get_backup_states());
            fprintf(stderr,"Error: fmi_on_storage push_to - did not push array\n");
        }
        else fprintf(stdout,"push_to past test second test\n");
        fprintf(stdout,"Test of fmi_on_storage end\n");

    }
};
} /* namespace fmi_on_storage*/
int main(){

    fmi_on_storage::FmuData fmudata;
    fmudata.test_functions();

  return 0;
}
