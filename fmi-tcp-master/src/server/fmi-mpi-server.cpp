#include "common/mpi_tools.h"
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
        int rank, tag;
        std::string recv_str = mpi_recv_string(MPI_ANY_SOURCE, &rank, &tag);

        //shutdown command?
        if (tag == 1) {
            break;
        }

        //let Server handle packet, send reply back to master
        std::string str = server.clientData(recv_str.c_str(), recv_str.length());
        MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, rank, tag, MPI_COMM_WORLD);
    }

    MPI_Finalize();

    return 0;
}
