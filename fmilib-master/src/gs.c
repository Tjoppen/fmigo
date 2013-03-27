#include <fmilib.h>
#include <stdio.h>

#include "main.h"
#include "utils.h"

int fmi1GaussSeidelStep(double time,
                        double communicationTimeStep,
                        int numFMUs,
                        fmi1_import_t ** fmus,
                        int numConnections,
                        connection connections[MAX_CONNECTIONS],
                        int numStepOrder,
                        int stepOrder[MAX_STEP_ORDER]){
    int i, j;

    for(i=0; i<numStepOrder; i++){
        int fmuIndex = stepOrder[i];

        fmi1_status_t s = fmi1_import_do_step (fmus[fmuIndex], time, communicationTimeStep, 1);
        if(s != fmi1_status_ok){
            printf("doStep() of FMU %d didn't return fmiOK! Exiting...\n",fmuIndex);
            return 1;
        }

        // Get all connections where this FMU is the "from" FMU
        for(j=0; j<numConnections; j++){
            if(connections[j].fromFMU == fmuIndex)
                fmi1TransferConnectionValues(connections[j], fmus);
        }
    }

    return 0;
}
