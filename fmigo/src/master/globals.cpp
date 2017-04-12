#include "master/globals.h"
#include <stdlib.h>

using namespace fmitcp_master;

namespace fmigo {
  namespace globals {
    FILEFORMAT fileFormat = csv;

    char getSeparator() {
      if (fileFormat == csv) {
        return ',';
      } else if (fileFormat == tikz) {
        return ' ';
      } else {
        fprintf(stderr, "Invalid file format: %i\n", fileFormat);
        exit(1);
      }
    }
  } //globals
} //fmigo
