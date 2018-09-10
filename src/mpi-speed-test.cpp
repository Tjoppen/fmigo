#include <mpi.h>
#include <memory.h>
//used for mpi_recv_string(), which isn't the fastest method but more fair
#include "common/mpi_tools.h"

int main(int argc, char *argv[]) {
    //These were gathered by running perftest2.sh in MPI mode
    //
    //  mpiexec -np 8 fmigo-mpi -m $method -t 10 -d 0.0005 -f none -a - $FMUS
    //
    
    /*
        mpich seems to by starting to oversubscribe at N == ncores:

        $ time mpiexec -np 7 ./mpi-speed-test
        real    0m0,153s
        user    0m0,845s
        sys     0m0,116s

        $ time mpiexec -np 8 ./mpi-speed-test
        real    0m0,296s  <--- ~68 kHz
        user    0m1,997s
        sys     0m0,088s

        $ time mpiexec -np 9 ./mpi-speed-test
        ^C
        real    1m43,083s
        user    12m36,525s
        sys     0m35,314s
     */

    //format: {size, count}
    //master sends packets of size sizes_out to all servers
    //servers reply with sizes_in size packets
    int sizes_out[][2] = {
        {6, 2},
        {12, 1},
        {38, 1},
        {45, 1},
        {714, 1},
        {746, 19999},
    };
    int sizes_in[][2] = {
        {6, 1},
        {16, 2},
        {22, 1},
        {71, 1},
        {85, 19999},
        {13756, 1},
    };
    //max of all packet sizes
    char data[13756];

    MPI_Init(NULL, NULL);

    //world = master at 0, FMUs at 1..N
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int N = world_size - 1;

    if (world_size != 8) {
        fprintf(stderr, "world_size %i != 8, results may be inaccurate\n", world_size);
    }

    if (world_rank == 0) {
        int out_ofs = 0;
        int pingpongs = 0;
        while ((size_t)out_ofs < sizeof(sizes_out)/sizeof(sizes_out[0])) {
            for (int x = 0; x < N; x++) {
                //fprintf(stderr, "send %i, %i B, %i left\n", x+1, sizes_out[out_ofs][0], sizes_out[out_ofs][1]);
                memset(data, x, sizes_out[out_ofs][0]);
                MPI_Send(data, sizes_out[out_ofs][0], MPI_CHAR, x+1, 0, MPI_COMM_WORLD);
            }

            for (int x = 0; x < N; x++) {
                //fprintf(stderr, "master recv %i\n", x+1);
                int rank, tag;
                std::string recv_str = mpi_recv_string(MPI_ANY_SOURCE, &rank, &tag);
            }

            pingpongs++;
            if (--sizes_out[out_ofs][1] == 0) {
                out_ofs++;
            }
        }
        fprintf(stderr, "master done\n");

        fprintf(stderr, "%i pingpongs\n", pingpongs);
    } else {
        int in_ofs = 0;
        while ((size_t)in_ofs < sizeof(sizes_in)/sizeof(sizes_in[0])) {
            int rank, tag;
            //fprintf(stderr, "server recv %i\n", world_rank);
            std::string recv_str = mpi_recv_string(0, &rank, &tag);
            //fprintf(stderr, "server recv %i, %zu B\n", world_rank, recv_str.length());
            memset(data, in_ofs, sizes_in[in_ofs][0]);
            MPI_Send(data, sizes_in[in_ofs][0], MPI_CHAR, 0, 0, MPI_COMM_WORLD);

            if (--sizes_in[in_ofs][1] == 0) {
                in_ofs++;
            }
        }
        fprintf(stderr, "server done\n");
    }
    
    MPI_Finalize();

    return 0;
}