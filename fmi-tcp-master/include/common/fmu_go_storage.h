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
 enum STORAGE{
     states = 1 << 1,
     indicators = 1 << 2,
     derivatives = 1 << 3,
     nominals = 1 << 4,
     bounds_i = states | derivatives | nominals
 };
class FmuGoStorage {
#define CREATE_DATA_HPP(name)                                           \
    private:                                                            \
        Storage m_##name;   /* to store the data */                     \
 public:                                                                \
    inline Data & get_current_##name() { return m_##name.first ;}       \
    inline Data & get_backup_##name()  { return m_##name.second;}       \
    inline double get_current_##name(size_t id, size_t index){                  \
        if(index > get_end(id,STORAGE::name)){                        \
            fprintf(stderr,"Error: out of range for Data::iterator get_"#name""); \
            exit(1);                                                    \
        }                                                               \
        return *(get_current_##name().begin()+get_offset(id,STORAGE::name)+index); \
    }                                                                   \
    inline double get_backup_##name(size_t id, size_t index){                  \
        if(index > get_end(id,STORAGE::name)){                          \
            fprintf(stderr,"Error: out of range for Data::iterator get_backup_"#name""); \
            exit(1);                                                    \
        }                                                               \
        return *(get_backup_##name().begin()+get_offset(id,STORAGE::name)+index); \
    }                                                                   \
    inline void get_current_##name(double *ret, size_t id) {                    \
        /*need do extract the correct dataset belonging to client*/     \
        size_t o = get_offset( id, STORAGE::name );                   \
        size_t e = get_end( id, STORAGE::name );                      \
        copy(get_current_##name().begin() + o,                                  \
             get_current_##name().begin() + e,                                  \
             ret + o);                                                  \
    }                                                                   \
    inline void get_backup_##name(double *ret, size_t id)  {            \
        /*need do extract the correct dataset belonging to client*/     \
        size_t o = get_offset( id, STORAGE::name );                   \
        size_t e = get_end( id, STORAGE::name );                      \
        copy(get_backup_##name().begin() + o,                           \
             get_backup_##name().begin() + e,                           \
             ret + o);                                                  \
    }

    CREATE_DATA_HPP(states);
    CREATE_DATA_HPP(indicators);
    CREATE_DATA_HPP(derivatives);
    CREATE_DATA_HPP(nominals);

#define CURRENT_BACKUP(name)                                            \
    inline Data& get_##name(enum STORAGE type){                         \
        switch (type) {                                                 \
        case STORAGE::states: return get_##name##_states();             \
        case STORAGE::derivatives: return get_##name##_derivatives();   \
        case STORAGE::indicators: return get_##name##_indicators();     \
        case STORAGE::nominals: return get_##name##_nominals();         \
        default:                                                        \
            /*gcc isn't quite smart enough to catch that this should never happen*/ \
            fprintf(stderr, "get_"#name"(): Unknown STORAGE: %d\n",type); \
            exit(1);                                                    \
        }                                                               \
    }

    CURRENT_BACKUP(current);
    CURRENT_BACKUP(backup);
    bool past_event(size_t id);

 private:
    BoundsVector m_bounds;
    BoundsVector m_bounds_i;
    inline BoundsVector & get_bounds(enum STORAGE type) {
        switch(type){
        case STORAGE::indicators:
            return m_bounds_i;
        case STORAGE::states:
        case STORAGE::derivatives:
        case STORAGE::nominals:
            return m_bounds;
        default:
            /*gcc isn't quite smart enough to catch that this should never happen*/
            fprintf(stderr, "get_bounds(): Unknown STORAGE: %d\n", type);
            exit(1);
        }

    }

    inline size_t & get_end(const size_t id, enum STORAGE type){return get_bounds(type).at(id).second;}

 public:
    inline size_t & get_offset(const size_t id, enum STORAGE type){return get_bounds(type).at(id).first;}
    inline size_t get_size(const size_t id, enum STORAGE type) {return get_end(id,type) - get_offset(id,type);}

    void allocate_storage_states(const vector<size_t> &size_vec,enum STORAGE type);
    void allocate_storage(const vector<size_t> &number_of_states,const vector<size_t> &number_of_indicators);
    ~FmuGoStorage();
    FmuGoStorage();
    FmuGoStorage(const vector<size_t> &number_of_states,const vector<size_t> &number_of_indicators);
    void print(enum STORAGE type,char *str);
    void print(enum STORAGE type);

    double absmin(enum STORAGE type);
    double absmin(enum STORAGE, size_t id);
    size_t size(enum STORAGE type);

    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param type Is one of enum STORAGE{states,indicators,derivatives,nominals}
     *  @param vec A vector with data
     */
    void push_to(size_t client_id, enum STORAGE type, const Data &vec);


    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param type Is one of enum STORAGE{states,indicators,derivatives,nominals}
     *  @param array An array with data
     */
    void push_to(size_t client_id, enum STORAGE type, double* array);


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
