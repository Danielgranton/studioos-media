#include "Timer.hpp"

Timer::Timer()
    : mStart(std::chrono::steady_clock::now())
{
}

void Timer::reset()
{
    mStart = std::chrono::steady_clock::now();
}

long long Timer::elapsedMilliseconds() const
{
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - mStart).count();
}
