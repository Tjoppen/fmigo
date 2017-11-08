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

//simple struct wherein one of r, i, b or s is set
//I'd make this a union, but std::string has a destructor
struct MultiValue {
    double r;
    int i;
    bool b;
    std::string s;

    MultiValue() : r(0), i(0), b(false) {
    }

    bool operator==(const MultiValue& a) const {
        return r==a.r && i==a.i && b==a.b && s==a.s;
    }
};

//converts a vector<MultiValue> to vector<T>, with the help of a member pointer of type T
template<typename T> std::vector<T> vectorToBaseType(const std::vector<MultiValue>& in, T MultiValue::*member) {
    std::vector<T> ret;
    ret.reserve(in.size());
    for (const MultiValue& it : in) {
        ret.push_back(it.*member);
    }
    return ret;
}

class WeakConnection {
public:
    connection conn;
    FMIClient *from;
    FMIClient *to;

    WeakConnection(const connection& conn, FMIClient *from, FMIClient *to);
    ~WeakConnection() {}

    //these perform automatic type conversion where appropriate
    MultiValue setFromReal(double in) const;
    MultiValue setFromInteger(int in) const;
    MultiValue setFromBoolean(bool in) const;
    MultiValue setFromString(std::string in) const;
};

class FMIClient;

//this type holds value references grouped by data type and client
typedef std::map<fmi2_base_type_enu_t, std::vector<int> > SendGetXType;
typedef std::map<FMIClient*, SendGetXType> OutputRefsType;
OutputRefsType getOutputWeakRefs(std::vector<WeakConnection> weakConnections);

//OK, this type is getting a bit complicated
//it collects value references and their associated type converted values, grouped by data type and client
typedef std::map<fmi2_base_type_enu_t, std::pair<std::vector<int>, std::vector<MultiValue> > > SendSetXType;
typedef std::map<FMIClient*, SendSetXType> InputRefsValuesType;
InputRefsValuesType getInputWeakRefsAndValues(std::vector<WeakConnection> weakConnections);
SendSetXType        getInputWeakRefsAndValues(std::vector<WeakConnection> weakConnections, FMIClient *client);

}

#endif	/* WEAKCONNECTION_H */

