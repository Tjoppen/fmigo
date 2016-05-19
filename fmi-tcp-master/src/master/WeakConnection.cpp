/* 
 * File:   WeakConnection.cpp
 * Author: thardin
 * 
 * Created on May 24, 2016, 4:04 PM
 */

#include "master/WeakConnection.h"

using namespace fmitcp_master;

fmitcp_master::WeakConnection::WeakConnection(const connection& conn, FMIClient *from, FMIClient *to) :
    conn(conn), from(from), to(to) {
}

/*fmitcp_master::WeakConnection::WeakConnection(const fmitcp_master::WeakConnection& orig) {
}

fmitcp_master::WeakConnection::~WeakConnection() {
}*/

