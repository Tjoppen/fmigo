#ifndef GS_H
#define GS_H

int gsStep(     double time,
                double communicationTimeStep,
                int numFMUs,
                fmi1_import_t ** fmus,
                int numConnections,
                connection connections[MAX_CONNECTIONS]);

#endif /* GS_H */