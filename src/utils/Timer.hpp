#pragma once

#include <chrono>

class Timer
{
public:
    Timer();

    void reset();
    long long elapsedMilliseconds() const;

private:
    std::chrono::steady_clock::time_point mStart;
};
