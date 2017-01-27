/**
 * Author: Jonas Sandqvist
 * Date: 18-01-2017
 */
#ifndef FMU_GO_STORAGE_HPP
#define FMU_GO_STORAGE_HPP
#include <iostream> // temp
#include <cstring>
#include<vector>

using namespace std;
/// Global data for FMUs as well as maps to fetch and read from the FMI/X
/// support.
///
/// Keep two state vectors so we can backup.
/// work with references so that we can go from backup to current without
/// copy.
namespace fmu_go_storage{
typedef pair< size_t, size_t>  Bounds;
typedef vector<double> Data;
typedef pair< Data, Data>  Storage;
typedef vector<Bounds> BoundsVector;
 enum STORAGE{states, indicators, derivatives, nominals};
class FmuGoStorage {
#define CREATE_DATA_HPP(name)                                           \
    private:                                                            \
        Storage m_##name;   /* to store the data */                     \
    inline Data & get_current_##name() { return m_##name.first ;}       \
 public:                                                                \
    inline Data & get_backup_##name()  { return m_##name.second;}       \
    inline Data & get_##name(){return get_current_##name();}            \
    inline double get_##name(size_t id, size_t index){                  \
        if(index > get_end(id,get_##name())){                           \
            fprintf(stderr,"Error: out of range for Data::iterator get_"#name""); \
            exit(1);                                                    \
        }                                                               \
        return *(get_##name().begin()+get_offset(id,get_##name())+index); \
    }                                                                   \
    inline double get_backup_##name(size_t id, size_t index){                  \
        if(index > get_end(id,get_backup_##name())){                           \
            fprintf(stderr,"Error: out of range for Data::iterator get_backup_"#name""); \
            exit(1);                                                    \
        }                                                               \
        return *(get_backup_##name().begin()+get_offset(id,get_backup_##name())+index); \
    }                                                                   \
    inline void get_##name(double *ret, size_t id) {                    \
        /*need do extract the correct dataset belonging to client*/     \
        size_t o = get_offset( id, get_##name() );                      \
        size_t e = get_end( id, get_##name() );                         \
        copy(get_##name().begin() + o,                                  \
             get_##name().begin() + e,                                  \
             ret);                                                      \
    }                                                                   \
    inline void get_backup_##name(double *ret, size_t id)  {            \
        /*need do extract the correct dataset belonging to client*/     \
        size_t o = get_offset( id, get_backup_##name() );               \
        size_t e = get_end( id, get_backup_##name() );                  \
        copy(get_backup_##name().begin() + o,                           \
             get_backup_##name().begin() + e,                           \
             ret);                                                      \
    }

    CREATE_DATA_HPP(states);
    CREATE_DATA_HPP(indicators);
    CREATE_DATA_HPP(derivatives);
    CREATE_DATA_HPP(nominals);

#define CURRENT_BACKUP(name)                                            \
    inline Data & get_##name(Data &p)                                   \
    {                                                                   \
        if( &p == &get_current_states()) return get_##name##_states();  \
        if( &p == &get_current_derivatives()) return get_##name##_derivatives(); \
        if( &p == &get_current_indicators()) return get_##name##_indicators(); \
        if( &p == &get_current_nominals()) return get_##name##_nominals(); \
    }

    CURRENT_BACKUP(current);
    CURRENT_BACKUP(backup);
    bool past_event(size_t id);

 private:
    BoundsVector m_bounds;
    BoundsVector m_bounds_i;
    inline BoundsVector & get_bounds(Data &p) {

        if(&p == &get_current_indicators() || &p == &get_backup_indicators())
            return m_bounds_i;
        if(&p == &get_current_states() || &p == &get_backup_states() ||
           &p == &get_current_derivatives() || &p == &get_backup_derivatives() ||
           &p == &get_current_nominals() || &p == &get_backup_nominals())
            return m_bounds;
        fprintf(stderr,"something is wrong\n");
        fprintf(stderr,"p = %p, \ns = %p\ni = %p\nd = %p\nn = %p\n",p,
                &get_current_states(),
                &get_current_indicators(),
                &get_current_derivatives(),
                &get_current_nominals()
                );
        exit(66);
    }

    inline STORAGE get_current_type(Data &p) {
        if( &p == &get_current_states()) return STORAGE::states;
        if( &p == &get_current_derivatives()) return STORAGE::derivatives;
        if( &p == &get_current_indicators()) return STORAGE::indicators;
        if( &p == &get_current_nominals()) return STORAGE::nominals;
    }
    inline Data& get_current(STORAGE e){
        if( e == STORAGE::states) return get_current_states();
        if( e == STORAGE::derivatives) return get_current_derivatives();
        if( e == STORAGE::indicators) return get_current_indicators();
        if( e == STORAGE::nominals) return get_current_nominals();
    }
    inline size_t & get_offset(const size_t s, Data &p){return get_bounds(p).at(s).first;}
    inline size_t & get_end(const size_t s,Data &p){return get_bounds(p).at(s).second;}

 public:
    inline size_t get_size(const size_t s, Data &p) {
        return get_bounds(p).at(s).second - get_bounds(p).at(s).first;
    }
    void allocate_storage_states(const vector<size_t> &size_vec,Data &p);
    void allocate_storage(const vector<size_t> &number_of_states, Data &p);
    void allocate_storage(const vector<size_t> &number_of_states,const vector<size_t> &number_of_indicators);
    ~FmuGoStorage();
    FmuGoStorage();
    FmuGoStorage(const vector<size_t> &number_of_states,const vector<size_t> &number_of_indicators);
    void print(Data & current,char *str);
    void print(Data & current);

    double absmin(Data & current);
    double absmin(Data & current, size_t id);
    size_t size(Data &p);

    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param current_data_set Is one of get_current_{states,indicators,derivatives}()
     *  @param vec A vector with data
     */
    void push_to(size_t client_id, Data &current_data_set, const Data &vec);


    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param current_data_set Is one of get_current_{states,indicators,derivatives}()
     *  @param array An array with data
     */
    void push_to(size_t client_id, Data &current, double* array);


    /** cycle(): maybe reset?
     *  Toggles the working dataset with a backup copy from last call of sync()
     *  swap only makes shallow copies
     */
    void cycle();


    /** sync():
     *  Makes a deep copy of the current dataset to a backup dataset
     */
    void sync();


    /** test_function():
     *  test the functionality of the class
     */
    void test_functions(void);

};
} /* namespace fmi_on_storage*/

#endif
