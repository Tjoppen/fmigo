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

std::string mpi_recv_string(int world_rank_in, int *world_rank_out, int *tag);

#endif	/* MPI_TOOLS_H */

