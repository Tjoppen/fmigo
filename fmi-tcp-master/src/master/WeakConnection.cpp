/* 
 * File:   WeakConnection.cpp
 * Author: thardin
 * 
 * Created on May 24, 2016, 4:04 PM
 */

#include <math.h>
#include "master/WeakConnection.h"

namespace fmitcp_master {

WeakConnection::WeakConnection(const connection& conn, FMIClient *from, FMIClient *to) :
    conn(conn), from(from), to(to) {
}

void WeakConnection::setFromReal(double in) {
    switch (conn.toType) {
    case fmi2_base_type_real:
        r = in*conn.slope + conn.intercept;
	break;
    case fmi2_base_type_int:
    case fmi2_base_type_enum:
        i = (int)(in*conn.slope + conn.intercept);
	break;
    case fmi2_base_type_bool:
        b = fabs(in * conn.slope + conn.intercept) > 0.5;
	break;
    case fmi2_base_type_str:
        //we could support this, but eh..
        fprintf(stderr, "Converting real -> str not supported\n");
        exit(1);
	break;
    }
}

void WeakConnection::setFromInteger(int in) {
    switch (conn.toType) {
    case fmi2_base_type_real:
        r = in*conn.slope + conn.intercept;
	break;
    case fmi2_base_type_int:
    case fmi2_base_type_enum:
        if (conn.slope == 1 && conn.intercept == 0) {
            //special case to avoid losing precision
            i = in;
        } else {
            i = (int)(in*conn.slope + conn.intercept);
        }
	break;
    case fmi2_base_type_bool:
        b = fabs(in*conn.slope + conn.intercept) > 0.5;
	break;
    case fmi2_base_type_str:
        //same here - let's avoid this for now
        fprintf(stderr, "Converting int -> str not supported\n");
        exit(1);
	break;
    }
}

void WeakConnection::setFromBoolean(bool in) {
    switch (conn.toType) {
    case fmi2_base_type_real:
        r = in*conn.slope + conn.intercept;
	break;
    case fmi2_base_type_int:
    case fmi2_base_type_enum:
        i = (int)(in*conn.slope + conn.intercept);
	break;
    case fmi2_base_type_bool:
        //slope/intercept on bool -> bool doesn't really make sense IMO
        if (conn.slope != 1 || conn.intercept != 0) {
            fprintf(stderr, "slope or intercept specified on bool -> bool connection doesn't make sense - stopping\n");
            exit(1);
        }
        b = in;
	break;
    case fmi2_base_type_str:
	break;
    }
}

void WeakConnection::setFromString(std::string in) {
    if (conn.toType != fmi2_base_type_str) {
        fprintf(stderr, "String outputs may only be connected to string inputs\n");
        exit(1);
    }

    s = in;
}

}
