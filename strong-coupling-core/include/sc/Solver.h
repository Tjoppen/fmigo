#ifndef SOLVER_H
#define SOLVER_H

#include "sc/Slave.h"
#include "sc/Constraint.h"
#include "sc/Connector.h"
#include <vector>

#define SCSOLVER_DEBUGPRINTS 0

namespace sc {

/// Holds all slaves and constraints in the system, and solves for constraint forces.
class Solver {

private:
    std::vector<Slave*> m_slaves;
    std::vector<Constraint*> m_constraints;
    std::vector<Connector*> m_connectors;

public:

    Solver();
    virtual ~Solver();

    /// Counter for connector index
    int m_connectorIndexCounter;

    /**
     * @brief Add a slave to the solver
     * @todo Adding a slave before adding its connectors to the slave leads to errors. Fix!
     * @param slave
     */
    void addSlave(Slave * slave);

    /**
     * @brief Add a constraint to the solver.
     * @param constraint
     */
    void addConstraint(Constraint * constraint);

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
    void getEquations(std::vector<Equation*> * eqs);

    /// Get a constraint
    Constraint * getConstraint(int i);

    /// Get a constraint
    int getNumConstraints();

    /**
     * @brief Solves the system of equation. Sets the constraint forces in each connector.
     */
    void solve(int printDebugInfo);
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

    /**
     * Updates all constraints.
     */
    void updateConstraints();
};

}

#endif
