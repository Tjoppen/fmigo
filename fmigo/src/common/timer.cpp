#include "common/timer.h"

fmigo::timer::timer() :
  m_time(std::chrono::high_resolution_clock::now()),
  dont_rotate(false) {
}

void fmigo::timer::rotate(const char* label) {
#ifdef FMIGO_PRINT_TIMINGS
  if (dont_rotate) return;

  std::chrono::high_resolution_clock::time_point t = std::chrono::high_resolution_clock::now();

  if (m_durations.find(label) == m_durations.end()) {
    m_durations[label] = 0;
  }

  m_durations[label] += std::chrono::duration<double, std::micro>(t - m_time).count();
  m_time = t;
#endif
}
