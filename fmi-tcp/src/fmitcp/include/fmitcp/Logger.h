#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <stdarg.h>
#include <stdlib.h>

namespace fmitcp {

    /// Logs stuff with a filter
    class Logger {

    private:

        /// OR'ed LogMessageTypes
        int m_filter;

        /// Stuff to print before each message
        std::string m_prefix;

    public:

        /// Log message filter types
        enum LogMessageType {
            LOG_DEBUG,
            LOG_NETWORK,
            LOG_ERROR
        };

        Logger();
        virtual ~Logger();

        /// Print to the log
        virtual void log(LogMessageType type, const char * format, ...);
        virtual void setFilter(int filter);
        virtual int getFilter() const;
        void setPrefix(std::string prefix);
    };

};

#endif
