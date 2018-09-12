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

OutputRefsType getOutputWeakRefs(vector<WeakConnection> weakConnections) {
    OutputRefsType weakRefs;

    for( auto wc: weakConnections){
        weakRefs[wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
    }

    return weakRefs;
}

template<typename T> T check(
        const WeakConnection& wc,
        const std::unordered_map<int,T>& values)
{
    auto it = values.find(wc.conn.fromOutputVR);
    if (it == values.end()) {
      fatal("VR %i was not requested\n", wc.conn.fromOutputVR);
    }
    return it->second;
}

void getInputWeakRefsAndValues_inner(const vector<WeakConnection>& weakConnections, bool have_cset, const fmitcp::int_set& cset,
        InputRefsValuesType& refValues) {
    for (const WeakConnection& wc : weakConnections) {
        if (have_cset && !cset.count(wc.to->m_id)) {
            //skip if we only want for some set of clients and wc.to isn't in it
            continue;
        }

        SendSetXType& target = refValues[wc.to];
        const connection& conn = wc.conn;

        switch (conn.toType) {
        case fmi2_base_type_real:
            target.real_vrs.push_back(conn.toInputVR);
            break;
        case fmi2_base_type_int:
            target.int_vrs.push_back(conn.toInputVR);
            break;
        case fmi2_base_type_bool:
            target.bool_vrs.push_back(conn.toInputVR);
            break;
        case fmi2_base_type_str:
            target.string_vrs.push_back(conn.toInputVR);
            break;
        case fmi2_base_type_enum:
            fatal("Tried to connect to enum input. Enums are not yet supported\n");
        }

        switch (conn.fromType) {
        case fmi2_base_type_real: {
            double in = check(wc, wc.from->m_reals);

            switch (conn.toType) {
            case fmi2_base_type_real:
                target.reals.push_back(in*conn.slope + conn.intercept);
                break;
            case fmi2_base_type_int:
                target.ints.push_back((int)(in*conn.slope + conn.intercept));
                break;
            case fmi2_base_type_bool:
                target.bools.push_back(fabs(in * conn.slope + conn.intercept) > 0.5);
                break;
            case fmi2_base_type_str:
                fatal("Converting real -> str not supported\n");
                break;
            case fmi2_base_type_enum:   //make compiler happy
                break;
            }

            break;
        }
        case fmi2_base_type_int: {
            int in = check(wc, wc.from->m_ints);

            switch (conn.toType) {
            case fmi2_base_type_real:
                target.reals.push_back(in*conn.slope + conn.intercept);
                break;
            case fmi2_base_type_int:
                if (conn.slope == 1 && conn.intercept == 0) {
                    //special case to avoid losing precision
                    target.ints.push_back(in);
                } else {
                    target.ints.push_back((int)(in*conn.slope + conn.intercept));
                }
                break;
            case fmi2_base_type_bool:
                target.bools.push_back(fabs(in*conn.slope + conn.intercept) > 0.5);
                break;
            case fmi2_base_type_str:
                //same here - let's avoid this for now
                fatal("Converting int -> str not supported\n");
                break;
            case fmi2_base_type_enum:   //make compiler happy
                break;
            }

            break;
        }
        case fmi2_base_type_bool: {
            bool in = check(wc, wc.from->m_bools);

            switch (conn.toType) {
            case fmi2_base_type_real:
                target.reals.push_back(in*conn.slope + conn.intercept);
                break;
            case fmi2_base_type_int:
                target.ints.push_back((int)(in*conn.slope + conn.intercept));
                break;
            case fmi2_base_type_bool:
                //slope/intercept on bool -> bool doesn't really make sense IMO
                if (conn.slope != 1 || conn.intercept != 0) {
                    fatal("slope or intercept specified on bool -> bool connection doesn't make sense - stopping\n");
                }
                target.bools.push_back(in);
                break;
            case fmi2_base_type_str:
                //this one too
                fatal("Converting bool -> str not supported\n");
                break;
            case fmi2_base_type_enum:   //make compiler happy
                break;
            }

            break;
        }
        case fmi2_base_type_str: {
            string in = check(wc, wc.from->m_strings);

            if (conn.toType != fmi2_base_type_str) {
                fatal("String outputs may only be connected to string inputs\n");
            }

            target.strings.push_back(in);

            break;
        }
        case fmi2_base_type_enum:
            fatal("Tried to connect enum output somewhere. Enums are not yet supported\n");
        }
    }
}

InputRefsValuesType getInputWeakRefsAndValues(const vector<WeakConnection>& weakConnections) {
    InputRefsValuesType refValues;
    getInputWeakRefsAndValues_inner(weakConnections, false, fmitcp::int_set(), refValues);
    return refValues;
}

InputRefsValuesType getInputWeakRefsAndValues(const vector<WeakConnection>& weakConnections, const fmitcp::int_set& cset) {
    InputRefsValuesType refValues;
    getInputWeakRefsAndValues_inner(weakConnections, true, cset, refValues);
    return refValues;
}

SendSetXType getInputWeakRefsAndValues(const vector<WeakConnection>& weakConnections, FMIClient *client) {
    fmitcp::int_set cset;
    cset.insert(client->m_id);
    InputRefsValuesType temp;
    getInputWeakRefsAndValues_inner(weakConnections, true, cset, temp);
    auto it = temp.find(client);
    if (it == temp.end()) {
        return SendSetXType();
    } else {
        return it->second;
    }
}

void getInputWeakRefsAndValues(const vector<WeakConnection>& weakConnections, const fmitcp::int_set& cset, InputRefsValuesType& refValues) {
    getInputWeakRefsAndValues_inner(weakConnections, true, cset, refValues);
}

}
