#ifndef EVENTPUMP_H_
#define EVENTPUMP_H_

#define lw_import
#include <lacewing.h>

namespace fmitcp {

    /**
     * @brief Ticks events forward in time. Should be shared among all event driven class instances.
     */
    class EventPump {

    private:

        lw_pump m_pump;

        /// True if the system is on its way to stop
        bool m_exiting;

    public:
        EventPump();
        ~EventPump();

        /// get the underlying lacewing pump
        lw_pump getPump();

        /// Lock the context and start ticking the event loop.
        void startEventLoop();

        /// Exit the event loop
        void exitEventLoop();

        /// Do one eventloop tick
        void tick();
    };

};

#endif
