#include "common/common.h"

static void printHelp(char *program_name, int *port) {
  //port as argument to see if it's non-NULL in parse_server_args()
  info("Usage: %s [OPTIONS] FMUPATH\n\
\n\
OPTIONS\n\
\n\
%s    -5 hdf5_filename\n\
        Dump outputs into HDF5 with given filename\n\
    --help\n\
        You're looking at it.\n\
\n\
FMUPATH\n\
\n\
    The path to the FMU to serve. If FMUPATH is \"dummy\", then the server will always respond with dummy responses, which is nice for debugging.\n\
\n",
    program_name,
    port ? "    --port [INTEGER]\n        The port to run the server on. Default is 3000.\n" : ""
  );
}


static void parse_server_args(int argc, char **argv, string *fmuPath,
        string *hdf5Filename, bool *debugLogging, jm_log_level_enu_t *log_level,
        int *port = NULL) {
  for (int j = 1; j < argc; j++) {
    string arg = argv[j];
    bool last = (j==argc-1);

    if (arg == "-h" || arg == "--help") {
      printHelp(argv[0], port);
      exit(EXIT_SUCCESS);

    } else if (arg == "-d" || arg == "--debugLogging") {
      *debugLogging = true;

    } else if (arg == "-v" || arg == "--version") {
      printf("%s\n",FMITCP_VERSION); // todo
      exit(EXIT_SUCCESS);

    } else if ((arg == "-l" || arg == "--logging") && !last) {
      *log_level = common::logOptionToJMLogLevel(argv[j+1]);
    } else if((arg == "--port" || arg == "-p") && !last) {
      if (!port) {
        printHelp(argv[0], port);
        exit(EXIT_FAILURE);
      }
      std::string nextArg = argv[j+1];

      std::istringstream ss(nextArg);
      ss >> *port;

      if (*port <= 0) {
        printf("Invalid port.\n");fflush(NULL);
        exit(EXIT_FAILURE);
      }

    } else if (arg == "-5" && !last) {
      *hdf5Filename = argv[++j];
    } else if (arg == "-D") {
      info("Always computing numerical directional derivatives, regardless of providesDirectionalDerivatives\n");
      alwaysComputeNumericalDirectionalDerivatives = true;
    } else {
      *fmuPath = argv[j];
    }
  }

  if (*fmuPath == "") {
    printHelp(argv[0], port);
    exit(EXIT_FAILURE);
  }
}
