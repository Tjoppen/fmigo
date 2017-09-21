#ifndef SOLVER_H
#define SOLVER_H

#include "sc/Slave.h"
#include "sc/Constraint.h"
#include "sc/Connector.h"
#include <vector>
#include <map>

#define SCSOLVER_DEBUGPRINTS 0

namespace sc {

/// Holds all slaves and constraints in the system, and solves for constraint forces.
class Solver {

private:
    std::vector<Slave*> m_slaves;
    std::vector<Constraint*> m_constraints;
    std::vector<Connector*> m_connectors;
    std::vector<Equation*> eqs;
    std::vector<double> rhs;
    std::vector<int> aSrow;
    std::vector<int> aScol;
    std::vector<double> aSval;
    std::vector<int> Ap;
    std::vector<int> Ai;
    std::vector<double> lambda;
    std::vector<double> Ax;
    bool equations_dirty;
    int nchangingentries, numsystemrows;
    std::vector<int> Srow;
    std::vector<int> Scol;
    std::vector<double> Sval;

    /// Spook parameter "a"
    double m_a;

    /// Spook parameter "b"
    double m_b;

    /// Spook parameter "epsilon"
    double m_epsilon;

    /// Time step
    double m_timeStep;

    //for internal use only
    void constructS();

public:
    std::vector<Equation*> getEquations();

    //sparse matrix of mobilities
    std::map<std::pair<int,int>, JacobianElement> m_mobilities;

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
    
    /// Get a constraint
    Constraint * getConstraint(int i);

    /// Get a constraint
    int getNumConstraints();

    /// Set everything up for solving. This may have complexity greater than O(N)
    void prepare();

    /**
     * @brief Solves the system of equation. Sets the constraint forces in each connector.
     */
    void solve(bool holonomic, int printDebugInfo);
    void solve(bool holonomic = true);

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
