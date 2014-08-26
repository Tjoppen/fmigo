#ifndef STRONGCONNECTION_H_
#define STRONGCONNECTION_H_

#include <sc/Constraint.h>
#include "Connection.h"

namespace fmitcp_master {

    /**
     * @brief Strong connection between two connectors.
     * The connection should be able to support any constraint from the strong-coupling library.
     */
    class StrongConnection {

    private:
        StrongConnector* m_connA;
        StrongConnector* m_connB;

        /// Constraint that connects the two connectors
        sc::Constraint * m_constraint;

    public:

        /// Constraint type
        enum StrongConnectionType {
            CONNECTION_LOCK
        };

        /// Create a strong coupling connection between connA and connB, of type type.
        StrongConnection( StrongConnector* connA , StrongConnector* connB, StrongConnectionType type );
        virtual ~StrongConnection();

        /// Get the strong coupling constraint
        sc::Constraint * getConstraint();
    };
};

#endif
