#include <mpi.h>
#include <memory.h>
#include <string>
#include <sys/time.h>
#include <stdlib.h>

//#define USE_ISEND //using MPI_Send() is actually faster on granular

//for dummying out memset()
//#define memset(...)

//copy-pasted from common/mpi_tools.h to make file more portable
//using the same code is more fair
static void mpi_recv_string(int world_rank_in, int *world_rank_out, int *tag, std::string &ret) {
    MPI_Status status, status2;
    int nbytes;

    //figure out how many bytes are incoming
    MPI_Probe(world_rank_in, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_CHAR, &nbytes);

    ret.resize(nbytes);
    MPI_Recv(&ret[0], nbytes, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status2);

    if (world_rank_out) *world_rank_out = status.MPI_SOURCE;
    if (tag)            *tag            = status.MPI_TAG;
}

static void delay(int us) {
    timeval tv, tv2;
    gettimeofday(&tv, NULL);

    tv.tv_usec = tv.tv_usec + us;
    if (tv.tv_usec > 1000000) {
        tv.tv_usec -= 1000000;
        tv.tv_sec++;
    }

    for (;;) {
        gettimeofday(&tv2, NULL);
        if (tv2.tv_usec >= tv.tv_usec &&
            tv2.tv_sec  >= tv.tv_sec) {
            break;
        }
    }
}

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

    MPI_Init(NULL, NULL);

    //world = master at 0, FMUs at 1..N
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int N = world_size - 1;

    if (world_size != 8) {
        fprintf(stderr, "world_size %i != 8, results may be inaccurate\n", world_size);
    }

    //fake delays in microseconds
    int fake_master = 0, fake_server = 0;
    if (argc >= 3) {
        fake_master = atoi(argv[1]);
        fake_server = atoi(argv[2]);
    }

    //max of all packet sizes
#define MAXSZ 13756
    char *data = (char*)malloc(MAXSZ*N);
    MPI_Request *requests = (MPI_Request*)malloc(N*sizeof(MPI_Request));
    std::string recv_str(MAXSZ, 0);

    for (int z = 0; z < 1; z++) {
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

    if (world_rank == 0) {
        int out_ofs = 0;
        int pingpongs = 0;
        while ((size_t)out_ofs < sizeof(sizes_out)/sizeof(sizes_out[0])) {
            memset(data, out_ofs, sizes_out[out_ofs][0]*N);
            for (int x = 0; x < N; x++) {
                //fprintf(stderr, "send %i, %i B, %i left\n", x+1, sizes_out[out_ofs][0], sizes_out[out_ofs][1]);
#ifdef USE_ISEND
                MPI_Isend(&data[x*sizes_out[out_ofs][0]], sizes_out[out_ofs][0], MPI_CHAR, x+1, 0, MPI_COMM_WORLD, &requests[x]);
#else
                MPI_Send(&data[x*sizes_out[out_ofs][0]], sizes_out[out_ofs][0], MPI_CHAR, x+1, 0, MPI_COMM_WORLD);
#endif
            }

            for (int x = 0; x < N; x++) {
                //fprintf(stderr, "master recv %i\n", x+1);
                int rank, tag;
                mpi_recv_string(MPI_ANY_SOURCE, &rank, &tag, recv_str);
            }

#ifdef USE_ISEND
            for (int x = 0; x < N; x++) {
                MPI_Status status;
                MPI_Wait(&requests[x], &status);
            }
#endif

            delay(fake_master);

            pingpongs++;
            if (--sizes_out[out_ofs][1] == 0) {
                out_ofs++;
            }
        }
        //fprintf(stderr, "master done\n");

        //fprintf(stderr, "%i pingpongs\n", pingpongs);
    } else {
        int in_ofs = 0;
        while ((size_t)in_ofs < sizeof(sizes_in)/sizeof(sizes_in[0])) {
            int rank, tag;
            //fprintf(stderr, "server recv %i\n", world_rank);
            mpi_recv_string(0, &rank, &tag, recv_str);
            //fprintf(stderr, "server recv %i, %zu B\n", world_rank, recv_str.length());

            //fake calculations
            delay(fake_server);

            memset(data, in_ofs, sizes_in[in_ofs][0]);
            MPI_Send(data, sizes_in[in_ofs][0], MPI_CHAR, 0, 0, MPI_COMM_WORLD);

            if (--sizes_in[in_ofs][1] == 0) {
                in_ofs++;
            }
        }
        //fprintf(stderr, "server done\n");
    }
    }

    free(requests);
    free(data);
    MPI_Finalize();

    return 0;
}