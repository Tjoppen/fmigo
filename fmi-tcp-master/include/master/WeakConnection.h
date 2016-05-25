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
};

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

}

#endif	/* WEAKCONNECTION_H */

