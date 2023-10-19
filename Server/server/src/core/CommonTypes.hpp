#pragma once

#include <chrono>

using hrclock = std::chrono::high_resolution_clock;
using hrc_time_point = hrclock::time_point;
using time_duration = std::chrono::duration<float>;
