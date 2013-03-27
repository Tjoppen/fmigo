#ifndef JACOBI_H
#define JACOBI_H

// Takes a Jacobi co-simulation step
int fmi1JacobiStep( double time,
                    double communicationTimeStep,
                    int numFMUs,
                    fmi1_import_t ** fmus,
                    int numConnections,
                    connection connections[MAX_CONNECTIONS]);
int fmi2JacobiStep( double time,
                    double communicationTimeStep,
                    int numFMUs,
                    fmi2_import_t ** fmus,
                    int numConnections,
                    connection connections[MAX_CONNECTIONS]);

#endif /* JACOBI_H */