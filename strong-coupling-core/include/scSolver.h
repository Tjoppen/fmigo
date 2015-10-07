#ifndef SCSOLVER_H
#define SCSOLVER_H

#include "scSlave.h"
#include "scConstraint.h"
#include "scConnector.h"
#include <vector>

#define SCSOLVER_DEBUGPRINTS 0

/**
 * Holds all slaves and constraints in the system, and solves for constraint
 * forces.
 */
class scSolver {

private:
    std::vector<scSlave*> m_slaves;
    std::vector<scConstraint*> m_constraints;
    std::vector<scConnector*> m_connectors;

public:

    scSolver();
    ~scSolver();

    /// Counter for connector index
    int m_connectorIndexCounter;

    /**
     * @brief Add a slave to the solver
     * @param slave
     */
    void addSlave(scSlave * slave);

    /**
     * @brief Add a constraint to the solver.
     * @param constraint
     */
    void addConstraint(scConstraint * constraint);

    /**
     * @brief Get the number of rows in the system matrix
     */
    int getSystemMatrixRows();

    /**
     * @brief Get number of columns in the system matrix.
     */
    int getSystemMatrixCols();

    /**
     * @brief Get all equations in a vector.
     * @param eqs Vector that will be appended with the equations.
     */
    void getEquations(std::vector<scEquation*> * eqs);

    /**
     * @brief Solves the system of equation. Sets the constraint forces in each connector.
     */
    void solve();

    /**
     * Set spook parameters for all equations at once.
     * @param relaxation
     * @param compliance
     * @param timeStep
     */
    void setSpookParams(double relaxation, double compliance, double timeStep);

    /**
     * Resets all constraint forces on all connnectors added.
     */
    void resetConstraintForces();
};

#endif
