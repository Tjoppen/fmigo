/* 
 * File:   mpi_tools.h
 * Author: thardin
 *
 * Created on December 30, 2015, 12:13 PM
 */

#ifndef MPI_TOOLS_H
#define	MPI_TOOLS_H

#include <mpi.h>
#include <string>

static std::string mpi_recv_string(int world_rank_in, int *world_rank_out, int *tag) {
    MPI_Status status, status2;
    int nbytes;

    //figure out how many bytes are incoming
    MPI_Probe(world_rank_in, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_CHAR, &nbytes);

    std::string ret(nbytes, 0);
    MPI_Recv(&ret[0], nbytes, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status2);

    if (world_rank_out) *world_rank_out = status.MPI_SOURCE;
    if (tag)            *tag            = status.MPI_TAG;

    return ret;
}

#endif	/* MPI_TOOLS_H */

