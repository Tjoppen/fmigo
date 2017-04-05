#include "common/mpi_tools.h"

#if defined(_WIN32)
#include <malloc.h>
#define alloca _alloca
#else
#include <alloca.h>
#endif

std::string mpi_recv_string(int world_rank_in, int *world_rank_out, int *tag) {
    MPI_Status status, status2;
    int nbytes;

    //figure out how many bytes are incoming
    MPI_Probe(world_rank_in, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_CHAR, &nbytes);

    //put received data on stack
    char *data = static_cast<char*>(alloca(nbytes));
    MPI_Recv(data, nbytes, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status2);
    
    if (world_rank_out) *world_rank_out = status.MPI_SOURCE;
    if (tag)            *tag            = status.MPI_TAG;
    
    return std::string(data, nbytes);
}
