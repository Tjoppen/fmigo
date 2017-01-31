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
void FmuGoStorage::allocate_storage_states(const vector<size_t> &size_vec, Data &data)
{
        size_t total_size = 0;
        size_t first = 0;
        Data &p = get_current_states();
        get_bounds(data).resize(0);
        for(auto n: size_vec){
            total_size += n;
            get_bounds(data).push_back(Bounds(first, first + n));
            first += n;
        }

        get_current(data).resize(0,0);
        get_current(data).resize(total_size,0);

        get_backup(get_current_states()) = get_current(get_current_states());
    }
void FmuGoStorage::allocate_storage(const vector<size_t> &size_vec, Data &p )
    {
        size_t total_size = 0;
        size_t first = 0;
        STORAGE type = get_current_type(p);
        get_bounds(p).resize(0);
        for(auto n: size_vec){
            total_size += n;
            get_bounds(p).push_back(Bounds(first, first + n));
            first += n;
        }

        get_current(p).resize(0,0);
        get_current(get_current(type)).resize(total_size,0);

        //allocate memory for storage of fmu_data
        get_backup(get_current(type)) = get_current(get_current(type));
    }
void FmuGoStorage::allocate_storage(const vector<size_t> &number_of_states,const vector<size_t> &number_of_indicators)
    {
        allocate_storage(number_of_states, get_current_states());
        allocate_storage(number_of_states, get_current_derivatives());
        allocate_storage(number_of_indicators, get_current_indicators());
        allocate_storage(number_of_states, get_current_nominals());
    }

void FmuGoStorage::print(Data & current){ char str[1]=""; FmuGoStorage::print(current,str);}
void FmuGoStorage::print(Data & current,char *str)
    {
        fprintf(stderr,str);
        for(auto x: get_bounds(current)) {
                for(size_t i = x.first; i != x.second; ++i)
                    fprintf(stderr,"%f ",current.at(i));
            }
        fprintf(stderr,"\n");
    }

double FmuGoStorage::absmin(Data &current){
    if(!current.size()) return 0;
    double min = abs(*(current.begin()));
    for(auto z: current)
        if(abs(z) < min)
            min = abs(z);
    return min;
}
double FmuGoStorage::absmin(Data &current,size_t id){
    size_t o = get_offset(id, current);
    double min = abs( *(current.begin() + o));
    for(size_t i = o + 1; i < get_end(id, current); i++)
        if(abs(*(current.begin() + i)) < min)
            min =abs(*(current.begin() + i));
    return min;
}

/** size()
 *  returns the number of fmus that have allocated memory
 */
size_t FmuGoStorage::size(Data &p){
    return get_bounds(p).size();
}

    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param current_data_set Is one of get_current_{states,indicators,derivatives}()
     *  @param vec A vector with data
     */
void FmuGoStorage::push_to(size_t client_id, Data &current_data_set, const Data &vec)
    {
        size_t s = get_size(client_id,current_data_set);


        if( s != vec.size()){
            fprintf(stderr,"error:FmuData-push_to(): client_id != vector.size!!\n");
            exit(1);
        }
        copy(vec.begin(),
             vec.end(),
             current_data_set.begin() + get_offset(client_id,current_data_set));
    }

    /** push_to():
     *  Replace the 'current_data_set' for 'client_id' with 'vec'
     *
     *  @param client_id The client id
     *  @param current_data_set Is one of get_current_{states,indicators,derivatives}()
     *  @param array An array with data
     */
void FmuGoStorage::push_to(size_t client_id, Data &current, double* array)
    {
        for(size_t i = get_offset(client_id,current); i < get_end(client_id,current); i++)
                current.at(i) = *array++;
    }

    /** cycle(): maybe reset?
     *  Toggles the working dataset with a backup copy from last call of sync()
     *  swap only makes shallow copies
     */
#define cycle_storage_cpp(name)\
    swap(get_##name(), get_backup_##name() );
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
    copy(get_##name().begin(), get_##name().end(), get_backup_##name().begin() );
void FmuGoStorage::sync(){
    sync_storage_cpp(states);
    sync_storage_cpp(derivatives);
    sync_storage_cpp(indicators);
    sync_storage_cpp(nominals);
}

bool FmuGoStorage::past_event(size_t id){
    Data & func = get_indicators();
    for(size_t index = get_offset(id, func); index < get_end(id, func);index++)
        {
        if(signbit( *(get_indicators().begin() + index)) !=
           signbit( *(get_backup_indicators().begin() + index))
           )
            {
            // fprintf(stderr,"compr past_event %f, %f \n",
            //         *(get_indicators().begin() + index),
            //         *(get_backup_indicators().begin() + index)
            //         );
            return true;
            }
        }
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
        for(int i = 0 ; i < n_##name.size(); ++i)\
            {\
            if( n_##name.at(i) !=\
                testFmuGoStorage.get_size(i ,\
                                          testFmuGoStorage.get_current_##name()))\
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
        testFmuGoStorage.push_to(0,testFmuGoStorage.get_current_##name(),Data(first_vec_storage_cpp));\
        testFmuGoStorage.push_to(2,testFmuGoStorage.get_current_##name(),Data(third_vec_storage_cpp));\
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

#define test_push_to_p_storage_cpp(name)\
        fprintf(stderr,"Testing push_to_p() -- "#name"  -  ");\
        testFmuGoStorage.push_to(0,testFmuGoStorage.get_backup_##name(),Data(first_vec_storage_cpp));\
        testFmuGoStorage.push_to(1,testFmuGoStorage.get_backup_##name(),Data(second_vec_storage_cpp));\
        testFmuGoStorage.push_to(2,testFmuGoStorage.get_backup_##name(),Data(third_vec_storage_cpp));\
        testFmuGoStorage.push_to(0,testFmuGoStorage.get_current_##name(),x);\
        testFmuGoStorage.push_to(1,testFmuGoStorage.get_current_##name(),y);\
        testFmuGoStorage.push_to(2,testFmuGoStorage.get_current_##name(),z);\
        a = testFmuGoStorage.get_backup_##name();\
        b = testFmuGoStorage.get_current_##name();\
        if(a != b) {\
            fprintf(stderr,"FAILD!\n");\
            print(testFmuGoStorage.get_current_##name());\
            print(testFmuGoStorage.get_backup_##name());\
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
#define test_get_p_storage_cpp(name)                                    \
        fprintf(stderr,"Testing get_"#name"_p()     -  ");              \
        testFmuGoStorage.get_##name(ret,2);                             \
        fail = false;                                                   \
        test_get_p_fail(z,                                              \
                        first_size_storage_cpp +                        \
                        second_size_storage_cpp,                        \
                        first_size_storage_cpp +                        \
                        second_size_storage_cpp +                       \
                        third_size_storage_cpp)                         \
        testFmuGoStorage.get_##name(ret,0);                             \
        test_get_p_fail(x,0, first_size_storage_cpp );                  \
                                                                        \
        testFmuGoStorage.get_##name(ret,1);                             \
        test_get_p_fail(y, first_size_storage_cpp ,                     \
                        first_size_storage_cpp +                        \
                        second_size_storage_cpp);                       \
                                                                        \
        k = 0;                                                      \
        for(auto v:get_##name())                                        \
            if(ret[k++] != v ) fail = true;                             \
        \
        if(fail) {                                                      \
            fprintf(stderr,"FAILD!\n");                                 \
            for(int i = 0; i < third_size_storage_cpp; i++)             \
                fprintf(stderr, "%f  ---  %f\n",z[i],ret[i]);           \
            fprintf(stderr,"print \n");                                 \
            testFmuGoStorage.print(testFmuGoStorage.get_##name());      \
            fail = true;                                                \
        } else fprintf(stderr,"OK!\n");

        test_get_p_storage_cpp(states);
        test_get_p_storage_cpp(backup_states);
        test_get_p_storage_cpp(derivatives);
        test_get_p_storage_cpp(backup_derivatives);
        test_get_p_storage_cpp(nominals);
        test_get_p_storage_cpp(backup_nominals);
        test_get_p_storage_cpp(indicators);
        test_get_p_storage_cpp(backup_indicators);

        fprintf(stderr,"Testing absmin()     -  ");
        if ( minVal  != testFmuGoStorage.absmin(testFmuGoStorage.get_indicators(),2) ||
             abs(minValn) != testFmuGoStorage.absmin(testFmuGoStorage.get_indicators(),1) ||
             abs(minValn) < minVal? abs(minValn):minVal != testFmuGoStorage.absmin(testFmuGoStorage.get_indicators()) )
            fprintf(stderr," Failed!\n  ");
        else
            fprintf(stderr," OK!\n");

    }
