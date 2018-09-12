/* 
 * File:   WeakConnection.h
 * Author: thardin
 *
 * Created on May 24, 2016, 4:04 PM
 */

#ifndef WEAKCONNECTION_H
#define	WEAKCONNECTION_H

#include "parseargs.h"

namespace fmitcp_master {

class FMIClient;

class WeakConnection {
public:
    connection conn;
    FMIClient *from;
    FMIClient *to;

    WeakConnection(const connection& conn, FMIClient *from, FMIClient *to);
    ~WeakConnection() {}
};

class FMIClient;

//this type holds value references grouped by data type and client
typedef std::map<fmi2_base_type_enu_t, std::vector<int> > SendGetXType;
typedef std::map<FMIClient*, SendGetXType> OutputRefsType;
OutputRefsType getOutputWeakRefs(std::vector<WeakConnection> weakConnections);

struct SendSetXType {
    std::vector<int> real_vrs;
    std::vector<int> int_vrs;
    std::vector<int> bool_vrs;
    std::vector<int> string_vrs;

    std::vector<double> reals;
    std::vector<int> ints;
    std::vector<bool> bools;
    std::vector<std::string> strings;
};

typedef std::map<FMIClient*, SendSetXType> InputRefsValuesType;

InputRefsValuesType getInputWeakRefsAndValues(const std::vector<WeakConnection>& weakConnections);
//only request values for clients whose IDs is in cset
InputRefsValuesType getInputWeakRefsAndValues(const std::vector<WeakConnection>& weakConnections, const fmitcp::int_set& cset);
SendSetXType        getInputWeakRefsAndValues(const std::vector<WeakConnection>& weakConnections, FMIClient *client);
void                getInputWeakRefsAndValues(const std::vector<WeakConnection>& weakConnections, const fmitcp::int_set& cset, InputRefsValuesType& refValues);

}

#endif	/* WEAKCONNECTION_H */

