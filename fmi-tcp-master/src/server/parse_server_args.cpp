static void printHelp(char *program_name, string *host, int *port) {
  //hostName and port as arguments to see if they're non-NULL in parse_server_args()
  fprintf(stderr, "Usage: %s [OPTIONS] FMUPATH\n\
\n\
OPTIONS\n\
\n\
%s%s    -F filter_depth\n\
        Depth of FIR filter applied to all outputs. Filter is disabled by default.\n\
    -5 hdf5_filename\n\
        Dump outputs into HDF5 with given filename\n\
    --help\n\
        You're looking at it.\n\
\n\
FMUPATH\n\
\n\
    The path to the FMU to serve. If FMUPATH is \"dummy\", then the server will always respond with dummy responses, which is nice for debugging.\n\
\n",
    program_name,
    port ? "    --port [INTEGER]\n        The port to run the server on. Default is 3000.\n" : "",
    host ? "    --host [STRING] \n        The host name to run the server on. Default is 'localhost'.\n" : ""
  );
}


static void parse_server_args(int argc, char **argv, string *fmuPath, int *filter_depth,
        string *hdf5Filename, bool *debugLogging, jm_log_level_enu_t *log_level,
        string *hostName = NULL, int *port = NULL) {
  for (int j = 1; j < argc; j++) {
    string arg = argv[j];
    bool last = (j==argc-1);

    if (arg == "-h" || arg == "--help") {
      printHelp(argv[0], hostName, port);
      exit(EXIT_SUCCESS);

    } else if (arg == "-d" || arg == "--debugLogging") {
      *debugLogging = true;

    } else if (arg == "-v" || arg == "--version") {
      printf("%s\n",FMITCP_VERSION); // todo
      exit(EXIT_SUCCESS);

    } else if ((arg == "-l" || arg == "--logging") && !last) {
      std::string nextArg = argv[j+1];

      std::istringstream ss(nextArg);
      int logging;
      ss >> logging;

      switch (logging) {
      case 0:
        *log_level = jm_log_level_nothing; break;
      case 1:
        *log_level = jm_log_level_fatal; break;
      case 2:
        *log_level = jm_log_level_error; break;
      case 3:
        *log_level = jm_log_level_warning; break;
      case 4:
        *log_level = jm_log_level_info; break;
      case 5:
        *log_level = jm_log_level_verbose; break;
      case 6:
        *log_level = jm_log_level_debug; break;
      case 7:
        *log_level = jm_log_level_all; break;
      default:
        fprintf(stderr, "Invalid logging. Possible options are from 0 to 7.\n");
        exit(EXIT_FAILURE);
      }

    } else if((arg == "--port" || arg == "-p") && !last) {
      if (!port) {
        printHelp(argv[0], hostName, port);
        exit(EXIT_FAILURE);
      }
      std::string nextArg = argv[j+1];

      std::istringstream ss(nextArg);
      ss >> *port;

      if (port <= 0) {
        printf("Invalid port.\n");fflush(NULL);
        exit(EXIT_FAILURE);
      }

    } else if (arg == "--host" && !last) {
      if (!hostName) {
        printHelp(argv[0], hostName, port);
        exit(EXIT_FAILURE);
      }
      *hostName = argv[j+1];
    } else if (arg == "-5" && !last) {
      *hdf5Filename = argv[++j];
    } else if (arg == "-F" && !last) {
      *filter_depth = atoi(argv[++j]);
      fprintf(stderr, "Using output filter with depth %i\n", *filter_depth);
    } else {
      *fmuPath = argv[j];
    }
  }
  
  if (*fmuPath == "") {
    printHelp(argv[0], hostName, port);
    exit(EXIT_FAILURE);
  }
}

