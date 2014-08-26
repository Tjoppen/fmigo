#include "Logger.h"
#include "stdio.h"
#include <string>
#include <stdlib.h>

using namespace fmitcp;

Logger::Logger(){
    m_filter = ~0; // Log everything
    m_prefix = "";
}

Logger::~Logger(){

}

void Logger::log(Logger::LogMessageType type, const char * format, ...){
    // Print prefix
    if(m_prefix != ""){
        printf("%s",m_prefix.c_str());
    }

    // Print message
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fflush(NULL);
}

void Logger::setFilter(int filter){
    m_filter = filter;
}

int Logger::getFilter() const {
    return m_filter;
}

void Logger::setPrefix(std::string prefix){
    m_prefix = prefix;
}
