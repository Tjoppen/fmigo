#ifndef GS_H
#define GS_H

/**
 * @brief Gauss-Seidel is a stepping method where we step the subsystems in serial. The order of connections will therefore matter.
 * @param time Current simulation time
 * @param communicationTimeStep Interval in seconds to exchange data in between FMUs
 * @param numFMUs
 * @param fmus
 * @param numConnections
 * @param connections
 */
int gsStep(     double time,
                double communicationTimeStep,
                int numFMUs,
                fmi1_import_t ** fmus,
                int numConnections,
                connection connections[MAX_CONNECTIONS]);

#endif /* GS_H */