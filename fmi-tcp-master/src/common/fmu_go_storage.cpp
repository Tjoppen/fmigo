/**
 * Author: Jonas Sandqvist
 * Date: 18-01-2017
 */
#include <iostream>
#include <cmath>
#ifdef DEBUG
#include "fmu_go_storage.hpp"
#else
#include "common/fmu_go_storage.h"
#endif
using namespace std;
/// Global data for FMUs as well as maps to fetch and read from the FMI/X
/// support.
///
/// Keep two state vectors so we can backup.
/// work with references so that we can go from backup to current without
/// copy.
using namespace fmu_go_storage;


FmuGoStorage::~FmuGoStorage()
{
}

FmuGoStorage::FmuGoStorage()
{
    //allocate_storage(vector<size_t>({}),get_current_states());
    //allocate_storage_states(vector<size_t>({}),get_current_states());
    allocate_storage(vector<size_t>({}),vector<size_t>({}));
}
FmuGoStorage::FmuGoStorage(const vector<size_t> &number_of_states,const vector<size_t> &number_of_indicators)
{
    //allocate_storage_states(number_of_states, get_current_states());
    //    allocate_storage(number_of_states, get_current_states());
    allocate_storage(number_of_states,number_of_indicators);
}
void FmuGoStorage::allocate_storage_states(const vector<size_t> &size_vec, enum STORAGE type)
{
        size_t total_size = 0;
        size_t first = 0;
        get_bounds(type).resize(0);
        for(auto n: size_vec){
            total_size += n;
            get_bounds(type).push_back(Bounds(first, first + n));
            first += n;
        }

        get_current(type).resize(0,0);
        get_current(type).resize(total_size,0);

        get_backup(type) = get_current(type);
    }

void FmuGoStorage::allocate_storage(const vector<size_t> &number_of_states,const vector<size_t> &number_of_indicators)
    {
        allocate_storage_states(number_of_states, STORAGE::states);
        allocate_storage_states(number_of_states, STORAGE::derivatives);
        allocate_storage_states(number_of_indicators, STORAGE::indicators);
        allocate_storage_states(number_of_states, STORAGE::nominals);
    }

void FmuGoStorage::print(enum STORAGE type){ char str[1]=""; FmuGoStorage::print(type,str);}
void FmuGoStorage::print(enum STORAGE type,char *str)
    {
        fputs(str,stderr);
        Data& current = get_current(type);
        Data& backup = get_backup(type);
        fprintf(stderr,"current \n");
        for(auto x: get_bounds(type)) {
                for(size_t i = x.first; i != x.second; ++i)
                    fprintf(stderr,"%f ",current.at(i));
            }
        fprintf(stderr,"\n");
        fprintf(stderr,"backup \n");
        for(auto x: get_bounds(type)) {
                for(size_t i = x.first; i != x.second; ++i)
                    fprintf(stderr,"%f ",backup.at(i));
            }
        fprintf(stderr,"\n");
    }

double FmuGoStorage::absmin(enum STORAGE type){

    if(!get_current(type).size()) return 0;
    double min = abs(*(get_current(type).begin()));
    for(auto z: get_current(type))
        if(abs(z) < min)
            min = abs(z);
    return min;
}
double FmuGoStorage::absmin(enum STORAGE type,size_t id){
    size_t o = get_offset(id, type);
    double min = abs( *(get_current(type).begin() + o));
    for(size_t i = o + 1; i < get_end(id, type); i++)
        if(abs(*(get_current(type).begin() + i)) < min)
            min =abs(*(get_current(type).begin() + i));
    return min;
}

/** size()
 *  returns the number of fmus that have allocated memory
 */
size_t FmuGoStorage::size(enum STORAGE type){
    return get_bounds(type).size();
}

    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param current_data_set Is one of get_current_{states,indicators,derivatives}()
     *  @param vec A vector with data
     */
void FmuGoStorage::push_to(size_t client_id, enum STORAGE type, const Data &vec)
    {
        size_t s = get_size(client_id, type);

        if( s != vec.size()){
            fprintf(stderr,"error:FmuData-push_to(): client_id != vector.size!!\n");
            exit(1);
        }
        copy(vec.begin(),
             vec.end(),
             get_current(type).begin() + get_offset(client_id,type));
    }

    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param current_data_set Is one of get_current_{states,indicators,derivatives}()
     *  @param array An array with data
     */
void FmuGoStorage::push_to(size_t client_id, enum STORAGE type, double* array)
    {
        for(size_t i = get_offset(client_id,type); i < get_end(client_id, type); i++)
            get_current(type).at(i) = *array++;
    }

    /** cycle(): maybe reset?
     *  Toggles the working dataset with a backup copy from last call of sync()
     *  swap only makes shallow copies
     */
#define cycle_storage_cpp(name)\
    swap(get_current_##name(), get_backup_##name() );
void FmuGoStorage::cycle() {
    cycle_storage_cpp(states);
    cycle_storage_cpp(derivatives);
    cycle_storage_cpp(indicators);
    cycle_storage_cpp(nominals);
}

    /** sync():
     *  Makes a deep copy of the current dataset to a backup dataset
     */
#define sync_storage_cpp(name)\
    copy(get_current_##name().begin(), get_current_##name().end(), get_backup_##name().begin() );
void FmuGoStorage::sync(){
    sync_storage_cpp(states);
    sync_storage_cpp(derivatives);
    sync_storage_cpp(indicators);
    sync_storage_cpp(nominals);
}

bool FmuGoStorage::past_event(size_t id){
    enum STORAGE type = STORAGE::indicators;
    size_t index = get_offset(id, type);
    for(; index < get_end(id, type); index++)
        if(signbit( *(get_current(type).begin() + index)) !=
           signbit( *(get_backup(type).begin() + index)))
            return true;
    return false;
}
    /** test_function():
     *  test the functionality of the class
     */
void FmuGoStorage::test_functions(void)
    {

        fprintf(stderr,"Test of fmi_on_storage start\n");
#define minVal 0.1
#define minValn -1
#define first_size_storage_cpp 1
#define second_size_storage_cpp 5
#define third_size_storage_cpp 11
#define first_vec_storage_cpp {1}
#define second_vec_storage_cpp {-3,-2,minValn,-4,-5}
#define third_vec_storage_cpp {1,2,3,4,5,6,7,8,9,10,minVal}
        vector<size_t> n_states({first_size_storage_cpp,
                    second_size_storage_cpp,
                    third_size_storage_cpp});
        vector<size_t> n_indicators = n_states;
        vector<size_t> n_derivatives = n_states;
        vector<size_t> n_nominals = n_states;
        Data a,b;

        FmuGoStorage testFmuGoStorage(n_states,n_indicators);
        bool fail = false;

#define test_size_storage_cpp(name)\
        fprintf(stderr,"Testing size() -- "#name"     -  ");\
        for(size_t i = 0 ; i < n_##name.size(); ++i)\
            {\
            if( n_##name.at(i) != testFmuGoStorage.get_size(i , STORAGE::name))   \
                fail = true;\
            }\
        if(fail)\
            fprintf(stderr,"FAILD!\n");\
        else fprintf(stderr,"OK!\n");

        test_size_storage_cpp(states);
        test_size_storage_cpp(derivatives);
        test_size_storage_cpp(indicators);
        test_size_storage_cpp(nominals);

#define test_push_to_storage_cpp(name)\
        fprintf(stderr,"Testing push_to()  -  ");\
        testFmuGoStorage.push_to(0,STORAGE::name,Data(first_vec_storage_cpp)); \
        testFmuGoStorage.push_to(2,STORAGE::name,Data(third_vec_storage_cpp)); \
        a = testFmuGoStorage.get_current_##name();\
        b = testFmuGoStorage.get_backup_##name();\
        if(a == b) fail = true;\
        if(fail){                                                       \
            fprintf(stderr,"FAILD push to -- "#name"!\n"); fail = false; \
        }                                                               \
        else fprintf(stderr,"OK!\n");

        test_push_to_storage_cpp(states);
        test_push_to_storage_cpp(derivatives);
        test_push_to_storage_cpp(indicators);
        test_push_to_storage_cpp(nominals);

#define test_cycle_storage_cpp_a(name)\
        Data & name##_a = testFmuGoStorage.get_backup_##name();
#define test_cycle_storage_cpp_b(name)\
        Data & name##_b = testFmuGoStorage.get_backup_##name();\
        fprintf(stderr,"Testing cycle() -- "#name" -  ");\
        if(name##_a != name##_b)\
            fprintf(stderr,"FAILD!\n");\
        else fprintf(stderr,"OK!\n");

        test_cycle_storage_cpp_a(states);
        test_cycle_storage_cpp_a(derivatives);
        test_cycle_storage_cpp_a(indicators);
        test_cycle_storage_cpp_a(nominals);
        testFmuGoStorage.cycle();
        test_cycle_storage_cpp_b(states);
        test_cycle_storage_cpp_b(derivatives);
        test_cycle_storage_cpp_b(indicators);
        test_cycle_storage_cpp_b(nominals);

        testFmuGoStorage.cycle();

#define test_sync_storage_cpp(name)\
        fprintf(stderr,"Testing sync() -- "#name"  -  ");\
        a = testFmuGoStorage.get_current_##name();\
\
        b = testFmuGoStorage.get_backup_##name();\
        if(a != b)\
            fprintf(stderr,"FAILD!\n");\
        else fprintf(stderr,"OK!\n");

        testFmuGoStorage.sync();
        test_sync_storage_cpp(states);
        test_sync_storage_cpp(derivatives);
        test_sync_storage_cpp(indicators);
        test_sync_storage_cpp(nominals);

        double x[first_size_storage_cpp] = first_vec_storage_cpp;
        double y[second_size_storage_cpp] = second_vec_storage_cpp;
        double z[third_size_storage_cpp] = third_vec_storage_cpp;

#define test_push_to_p_storage_cpp(name)                                \
        fprintf(stderr,"Testing push_to_p() -- "#name"  -  ");          \
        testFmuGoStorage.push_to(0,STORAGE::name,Data(first_vec_storage_cpp)); \
        testFmuGoStorage.push_to(1,STORAGE::name,Data(second_vec_storage_cpp)); \
        testFmuGoStorage.push_to(2,STORAGE::name,Data(third_vec_storage_cpp)); \
        testFmuGoStorage.cycle();                                       \
        testFmuGoStorage.push_to(0,STORAGE::name,x);                    \
        testFmuGoStorage.push_to(1,STORAGE::name,y);                    \
        testFmuGoStorage.push_to(2,STORAGE::name,z);                    \
        a = testFmuGoStorage.get_backup_##name();                       \
        b = testFmuGoStorage.get_current_##name();                      \
        if(a != b) {                                                    \
            fprintf(stderr,"FAILD!\n");                                 \
            print(STORAGE::name);                                       \
            print(STORAGE::name);                                       \
        }\
        else fprintf(stderr,"OK!\n");

        test_push_to_p_storage_cpp(states);
        test_push_to_p_storage_cpp(nominals);
        test_push_to_p_storage_cpp(derivatives);
        test_push_to_p_storage_cpp(indicators);

        size_t s = first_size_storage_cpp +
                   second_size_storage_cpp +
                   third_size_storage_cpp;
        double ret[s];
        int j ;
#define test_get_p_fail(name, s1, s2)                           \
        j = 0;                                                  \
        for(int i = s1; i < s2 ; j++,i++)                       \
            if(name[j] != ret[i]) {                            \
                fail = true;                                    \
                break;                                          \
            }                                                   \
        if(fail){                                               \
            fprintf(stderr,"FAILD! "#name"\n");                 \
            j = 0;                                              \
            cout << "s1 " << s1 << " s2 " << s2 << endl;        \
            for(int i = 0; i < s2 ; i++)                        \
                cout << ret[i] << " ";                          \
            cout <<  endl;                                      \
            for(int i = s1; i < s2 ; j++,i++)                   \
                cout << name[j] << " --- " << ret[i] << endl;  \
        }


        int k;
#define test_get_p_storage_cpp(type,name)                               \
        fprintf(stderr,"Testing get_"#type#name"_p()     -  ");              \
        testFmuGoStorage.get_##type##_##name(ret,2);                            \
        fail = false;                                                   \
        test_get_p_fail(z,                                              \
                        first_size_storage_cpp +                        \
                        second_size_storage_cpp,                        \
                        first_size_storage_cpp +                        \
                        second_size_storage_cpp +                       \
                        third_size_storage_cpp)                         \
        testFmuGoStorage.get_##type##_##name(ret,0);                             \
        test_get_p_fail(x,0, first_size_storage_cpp );                  \
                                                                        \
        testFmuGoStorage.get_##type##_##name(ret,1);                             \
        test_get_p_fail(y, first_size_storage_cpp ,                     \
                        first_size_storage_cpp +                        \
                        second_size_storage_cpp);                       \
                                                                        \
        k = 0;                                                      \
        for(auto v:get_##type##_##name())                                        \
            if(ret[k++] != v ) fail = true;                             \
        \
        if(fail) {                                                      \
            fprintf(stderr,"FAILD!\n");                                 \
            for(int i = 0; i < third_size_storage_cpp; i++)             \
                fprintf(stderr, "%f  ---  %f\n",z[i],ret[i]);           \
            fprintf(stderr,"print \n");                                 \
            testFmuGoStorage.print(STORAGE::name);                      \
            fail = true;                                                \
        } else fprintf(stderr,"OK!\n");

        test_get_p_storage_cpp(current,states);
        // test_get_p_storage_cpp(current,derivatives);
        // test_get_p_storage_cpp(current,nominals);
        // test_get_p_storage_cpp(current,indicators);
        // test_get_p_storage_cpp(backup,states);
        // test_get_p_storage_cpp(backup,derivatives);
        // test_get_p_storage_cpp(backup,nominals);
        // test_get_p_storage_cpp(backup,indicators);

        fprintf(stderr,"Testing absmin()     -  ");
        if ( minVal  != testFmuGoStorage.absmin(STORAGE::indicators,2) ||
             abs(minValn) != testFmuGoStorage.absmin(STORAGE::indicators,1) ||
             abs(minValn) < minVal? abs(minValn):minVal != testFmuGoStorage.absmin(STORAGE::indicators) )
            fprintf(stderr," Failed!\n  ");
        else
            fprintf(stderr," OK!\n");

    }
