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
    double r;
    int i;
    bool b;
    std::string s;

public:
    connection conn;
    FMIClient *from;
    FMIClient *to;

    WeakConnection(const connection& conn, FMIClient *from, FMIClient *to);
    ~WeakConnection() {}

    //these perform automatic type conversion where appropriate
    void setFromReal(double in);
    void setFromInteger(int in);
    void setFromBoolean(bool in);
    void setFromString(std::string in);
};

}

#endif	/* WEAKCONNECTION_H */

