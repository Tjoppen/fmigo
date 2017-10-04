#ifndef FMIGO_TIMER_H
#define FMIGO_TIMER_H

#include <chrono>
#include <map>

namespace fmigo {
  class timer {
  public:
    //time marker
    std::chrono::high_resolution_clock::time_point m_time;
    //durations in µs
    std::map<std::string, double> m_durations;
    //if true, ignore calls to rotate()
    bool dont_rotate;

    timer();

    //measures current time, logs duration since last time under given label
    void rotate(std::string label);
  };
}

#endif //FMIGO_TIMER_H
