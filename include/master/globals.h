#ifndef GLOBALS_H
#define GLOBALS_H

#include "master/parseargs.h"  //for FILEFORMAT
#include "common/timer.h"

namespace fmigo {
  namespace globals {
    /**
     * Globals that are needed all over the program should go here, so they
     * don't need to be copied all over the place.
     * Candidates for globals are loglevels, fileformats.. Anything that can't
     * just live in main() and that doesn't live inside a pointer.
     */

    //output file formats. csv, tikz
    extern fmitcp_master::FILEFORMAT fileFormat;

    //output file (default = stdout)
    extern FILE *outfile;

    extern fmigo::timer timer;

    /**
     * @brief Returns the separator used for the current file format.
     * For CSV this is comma, for TikZ this is space.
     * @return char with current file format separator
     */
    char getSeparator();
  }
}


#endif //GLOBALS_H
