#include <stdio.h>
#include "gsl-interface.h"

int main(void){
    cgsl_model m;
    m.n_variables = 1;
    cgsl_simulation sim = cgsl_init_simulation(&m,1,1e-10,0,0,0,NULL);
    return 0;
}
