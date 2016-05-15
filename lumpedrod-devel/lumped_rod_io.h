#ifndef LUMPED_ROD_IO
#define LUMPED_ROD_IO

#include <lumped_rod.h>

#ifdef __cplusplus
exterm "C" { 
#endif
void write_lumped_rod_config( const char * filename, const char * groupname, lumped_rod_sim * sim);

void write_lumped_rod_states( const char * filename, const char * groupname, lumped_rod_sim * sim);
#ifdef __cplusplus
}
#endif

#endif
