#include "master/globals.h"
#include <stdlib.h>

using namespace fmitcp_master;

namespace fmigo {
  namespace globals {
    FILEFORMAT fileFormat = csv;
    fmigo::timer timer;

    char getSeparator() {
      switch (fileFormat) {
      case csv:  return ',';
      case tikz: return ' ';
      default:   return '\0';
      }
    }
  } //globals
} //fmigo
