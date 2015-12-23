#include <mpi.h>
#include "server/FMIServer.h"

using namespace std;
using namespace fmitcp;

int main(int argc, char *argv[]) {
    MPI_Init(NULL, NULL);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    //init
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s fmu-path\n", argv[0]);
        return 1;
    }

    FMIServer server(argv[1], false, jm_log_level_nothing, "");

    for (;;) {
        MPI_Status status, status2;
        int nbytes;

        //figure out how many bytes are incoming
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_CHAR, &nbytes);

        //put received data on stack
        char *data = static_cast<char*>(alloca(nbytes));
        MPI_Recv(data, nbytes, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status2);

        //let Server handle packet, send reply back to client
        std::string str = server.clientData(data, nbytes);
        MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD);
    }

    return 0;
}
