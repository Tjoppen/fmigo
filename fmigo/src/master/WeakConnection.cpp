/*
 * File:   WeakConnection.cpp
 * Author: thardin
 *
 * Created on May 24, 2016, 4:04 PM
 */

#include <math.h>
#include <typeinfo>
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
        fatal("Converting real -> str not supported\n");
    case fmi2_base_type_enum:
        fatal("Converting real -> enum not supported\n");
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
        fatal("Converting int -> str not supported\n");
    case fmi2_base_type_enum:
        fatal("Converting int -> enum not supported\n");
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
            fatal("slope or intercept specified on bool -> bool connection doesn't make sense - stopping\n");
        }
        ret.b = in;
        break;
    case fmi2_base_type_str:
        //this one too
        fatal("Converting bool -> str not supported\n");
    case fmi2_base_type_enum:
        fatal("Converting bool -> enum not supported\n");
    }
    return ret;
}

MultiValue WeakConnection::setFromString(std::string in) const {
    MultiValue ret;
    if (conn.toType != fmi2_base_type_str) {
        fatal("String outputs may only be connected to string inputs\n");
    }

    ret.s = in;
    return ret;
}

OutputRefsType getOutputWeakRefs(vector<WeakConnection> weakConnections) {
    OutputRefsType weakRefs;

    for( auto wc: weakConnections){
        weakRefs[wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
    }

    return weakRefs;
}

template<typename T> void doit(
        InputRefsValuesType& refValues,
        WeakConnection& wc,
        const std::unordered_map<int,T>& values,
        MultiValue (WeakConnection::*convert)(T) const)
{
    auto it = values.find(wc.conn.fromOutputVR);
    if (it == values.end()) {
      fatal("VR %i was not requested\n", wc.conn.fromOutputVR);
    }

    MultiValue value = (wc.*convert)(it->second);

    refValues[wc.to][wc.conn.toType].first.push_back(wc.conn.toInputVR);
    refValues[wc.to][wc.conn.toType].second.push_back(value);
}

static InputRefsValuesType getInputWeakRefsAndValues_internal(vector<WeakConnection> weakConnections, FMIClient *toClient) {
    InputRefsValuesType refValues; //VRs and corresponding values for each client

    for (size_t x = 0; x < weakConnections.size(); x++) {
        WeakConnection& wc = weakConnections[x];

        if (toClient && wc.to != toClient) {
            //skip if we only want for a specific client
            continue;
        }

        switch (wc.conn.fromType) {
        case fmi2_base_type_real:
            doit(refValues, wc, wc.from->m_reals,    &WeakConnection::setFromReal);
            break;
        case fmi2_base_type_int:
            doit(refValues, wc, wc.from->m_ints,     &WeakConnection::setFromInteger);
            break;
        case fmi2_base_type_bool:
            doit(refValues, wc, wc.from->m_bools,    &WeakConnection::setFromBoolean);
            break;
        case fmi2_base_type_str:
            doit(refValues, wc, wc.from->m_strings,  &WeakConnection::setFromString);
            break;
        case fmi2_base_type_enum:
            fatal("Tried to connect enum output somewhere. Enums are not yet supported\n");
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
