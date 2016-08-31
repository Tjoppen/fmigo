/* 
 * File:   WeakConnection.cpp
 * Author: thardin
 * 
 * Created on May 24, 2016, 4:04 PM
 */

#include <math.h>
#include "master/WeakConnection.h"
#include "master/FMIClient.h"
#include <typeinfo>

using namespace std;

namespace fmitcp_master {

WeakConnection::WeakConnection(const connection& conn, FMIClient *from, FMIClient *to) :
    conn(conn), from(from), to(to) {
}

MultiValue WeakConnection::setFromReal(double in) const {
    MultiValue ret;
    switch (conn.toType) {
    case fmi2_base_type_real:
        ret.r = in*conn.slope + conn.intercept;
	break;
    case fmi2_base_type_int:
        ret.i = (int)(in*conn.slope + conn.intercept);
	break;
    case fmi2_base_type_bool:
        ret.b = fabs(in * conn.slope + conn.intercept) > 0.5;
	break;
    case fmi2_base_type_str:
        //we could support this, but eh..
        fprintf(stderr, "Converting real -> str not supported\n");
        exit(1);
	break;
    }
    return ret;
}

MultiValue WeakConnection::setFromInteger(int in) const {
    MultiValue ret;
    switch (conn.toType) {
    case fmi2_base_type_real:
        ret.r = in*conn.slope + conn.intercept;
	break;
    case fmi2_base_type_int:
        if (conn.slope == 1 && conn.intercept == 0) {
            //special case to avoid losing precision
            ret.i = in;
        } else {
            ret.i = (int)(in*conn.slope + conn.intercept);
        }
	break;
    case fmi2_base_type_bool:
        ret.b = fabs(in*conn.slope + conn.intercept) > 0.5;
	break;
    case fmi2_base_type_str:
        //same here - let's avoid this for now
        fprintf(stderr, "Converting int -> str not supported\n");
        exit(1);
	break;
    }
    return ret;
}

MultiValue WeakConnection::setFromBoolean(bool in) const {
    MultiValue ret;
    switch (conn.toType) {
    case fmi2_base_type_real:
        ret.r = in*conn.slope + conn.intercept;
	break;
    case fmi2_base_type_int:
        ret.i = (int)(in*conn.slope + conn.intercept);
	break;
    case fmi2_base_type_bool:
        //slope/intercept on bool -> bool doesn't really make sense IMO
        if (conn.slope != 1 || conn.intercept != 0) {
            fprintf(stderr, "slope or intercept specified on bool -> bool connection doesn't make sense - stopping\n");
            exit(1);
        }
        ret.b = in;
	break;
    case fmi2_base_type_str:
        //this one too
        fprintf(stderr, "Converting bool -> str not supported\n");
        exit(1);
	break;
    }
    return ret;
}

MultiValue WeakConnection::setFromString(std::string in) const {
    MultiValue ret;
    if (conn.toType != fmi2_base_type_str) {
        fprintf(stderr, "String outputs may only be connected to string inputs\n");
        exit(1);
    }

    ret.s = in;
    return ret;
}

OutputRefsType getOutputWeakRefs(vector<WeakConnection> weakConnections) {
    OutputRefsType weakRefs;

    for (size_t x = 0; x < weakConnections.size(); x++) {
        WeakConnection wc = weakConnections[x];
        weakRefs[wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
    }

    return weakRefs;
}

template<typename T> void doit(
        InputRefsValuesType& refValues,
        WeakConnection& wc,
        map<FMIClient*, size_t>& valueOfs,
        const deque<T>& values,
        MultiValue (WeakConnection::*convert)(T) const)
{
    size_t ofs = valueOfs[wc.from];

    if (ofs >= values.size()) {
        //shouldn't happen
        fprintf(stderr, "Number of setX() doesn't match number of getX() (%zu vs %zu) '%s'\n", ofs, values.size(), typeid(T).name());
        exit(1);
    }

    MultiValue value = (wc.*convert)(values[ofs]);

    refValues[wc.to][wc.conn.toType].first.push_back(wc.conn.toInputVR);
    refValues[wc.to][wc.conn.toType].second.push_back(value);
    valueOfs[wc.from]++;

}

static InputRefsValuesType getInputWeakRefsAndValues_internal(vector<WeakConnection> weakConnections, FMIClient *toClient) {
    InputRefsValuesType refValues; //VRs and corresponding values for each client

    //for keeping track of where we are in each FMIClient->m_getXValues
    map<FMIClient*, size_t> realValueOfs;
    map<FMIClient*, size_t> integerValueOfs;
    map<FMIClient*, size_t> booleanValueOfs;
    map<FMIClient*, size_t> stringValueOfs;

    for (size_t x = 0; x < weakConnections.size(); x++) {
        WeakConnection& wc = weakConnections[x];

        if (toClient && wc.to != toClient) {
            //skip if we only want for a specific client
            continue;
        }

        switch (wc.conn.fromType) {
        case fmi2_base_type_real:
            doit(refValues, wc, realValueOfs,    wc.from->m_getRealValues,    &WeakConnection::setFromReal);
            break;
        case fmi2_base_type_int:
            doit(refValues, wc, integerValueOfs, wc.from->m_getIntegerValues, &WeakConnection::setFromInteger);
            break;
        case fmi2_base_type_bool:
            doit(refValues, wc, booleanValueOfs, wc.from->m_getBooleanValues, &WeakConnection::setFromBoolean);
            break;
        case fmi2_base_type_str:
            doit(refValues, wc, stringValueOfs,  wc.from->m_getStringValues,  &WeakConnection::setFromString);
            break;
        }
    }

    return refValues;
}

InputRefsValuesType getInputWeakRefsAndValues(vector<WeakConnection> weakConnections) {
    return getInputWeakRefsAndValues_internal(weakConnections, NULL);
}

SendSetXType getInputWeakRefsAndValues(vector<WeakConnection> weakConnections, FMIClient *client) {
    InputRefsValuesType temp = getInputWeakRefsAndValues_internal(weakConnections, client);
    auto it = temp.find(client);
    if (it == temp.end()) {
        return SendSetXType();
    } else {
        return it->second;
    }
}

}
