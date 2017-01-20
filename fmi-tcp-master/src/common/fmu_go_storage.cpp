/**
 * Author: Jonas Sandqvist
 * Date: 18-01-2017
 */
#include <iostream>
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
    }

void FmuGoStorage::print(Data & current)
    {
        for(auto x: get_bounds(current)) {
                for(size_t i = x.first; i != x.second; ++i)
                    fprintf(stdout,"%f ",current.at(i));
                fprintf(stdout,"\n");
            }
        fprintf(stdout,"\n");
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
void FmuGoStorage::cycle()
    {
        swap(m_states.first, m_states.second );
        swap(m_indicators.first, m_indicators.second );
        swap(m_derivatives.first, m_derivatives.second );
    }

    /** sync():
     *  Makes a deep copy of the current dataset to a backup dataset
     */
inline void FmuGoStorage::sync(){
    get_current_states().begin();
    get_current_states().end();

        copy(m_states.first.begin(), m_states.first.end(), m_states.second.begin() );
        copy(m_indicators.first.begin(), m_indicators.first.end(), m_indicators.second.begin() );
        copy(m_derivatives.first.begin(), m_derivatives.first.end(), m_derivatives.second.begin() );
    }

    /** test_function():
     *  test the functionality of the class
     */
void FmuGoStorage::test_functions(void)
    {

        fprintf(stdout,"Test of fmi_on_storage start\n");
        vector<size_t> n_states({1,2,3});
        vector<size_t> n_indicators({1,2,3});
        Data a,b;

        FmuGoStorage testFmuGoStorage(n_states,n_indicators);
        fprintf(stderr,"Testing size()     -  ");
        bool fail = false;

        for(int i = 0 ; i < n_states.size(); ++i)
            {
            if( n_states.at(i) != testFmuGoStorage.get_size(i,testFmuGoStorage.get_current_states()))
                fail = true;
            }

        for(int i = 0 ; i < n_indicators.size(); ++i)
            if( n_indicators.at(i) != testFmuGoStorage.get_size(i,testFmuGoStorage.get_current_states()))
                fail = true;

        if(fail)
            fprintf(stderr,"FAILD!\n");
        else fprintf(stdout,"OK!\n");

        testFmuGoStorage.push_to(0,testFmuGoStorage.get_current_states(),Data({2}));
        testFmuGoStorage.push_to(2,testFmuGoStorage.get_current_states(),Data({2,3,5}));
        a = testFmuGoStorage.get_current_states();
        b = testFmuGoStorage.get_backup_states();
        fprintf(stdout,"Testing push_to()  -  ");
        if(a == b)
            fprintf(stderr,"FAILD!\n");
        else fprintf(stdout,"OK!\n");

        testFmuGoStorage.cycle();
        b = testFmuGoStorage.get_backup_states();
        fprintf(stdout,"Testing cycle()    -  ");
        if(a != b)
            fprintf(stderr,"FAILD!\n");
        else fprintf(stdout,"OK!\n");

        testFmuGoStorage.cycle();
        fprintf(stderr,"Testing sync()     -  ");
        testFmuGoStorage.sync();
        a = testFmuGoStorage.get_current_states();
        b = testFmuGoStorage.get_backup_states();
        if(a != b)
            fprintf(stderr,"FAILD!\n");
        else fprintf(stdout,"OK!\n");

        testFmuGoStorage.push_to(0,testFmuGoStorage.get_backup_states(),Data({5}));
        testFmuGoStorage.push_to(1,testFmuGoStorage.get_backup_states(),Data({5,351}));
        testFmuGoStorage.push_to(2,testFmuGoStorage.get_backup_states(),Data({5,684,5135}));
        double x[1] = {5};
        double y[2] = {5,351};
        double z[3] = {5,684,5135};

        testFmuGoStorage.push_to(0,testFmuGoStorage.get_current_states(),x);
        testFmuGoStorage.push_to(1,testFmuGoStorage.get_current_states(),y);
        testFmuGoStorage.push_to(2,testFmuGoStorage.get_current_states(),z);
        a = testFmuGoStorage.get_backup_states();
        b = testFmuGoStorage.get_current_states();
        fprintf(stdout,"Testing push_to()  -  ");
        if(a != b) {
            fprintf(stderr,"FAILD!\n");
            print(testFmuGoStorage.get_current_states());
            print(testFmuGoStorage.get_backup_states());
        }
        else fprintf(stdout,"OK!\n");


    }
