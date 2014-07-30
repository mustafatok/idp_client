#include "timer.h"

#include <iostream>
#include <string.h>
#include <time.h>

using namespace std;

#define NS          1000000000      // 10^9
#define NS2MS       1000000         // 10^6
#define NS2YS       1000            // 10^3

int64_t Timer::diff_ms()
{
        return diff_ns() / NS2MS;
}

int64_t Timer::diff_ys()
{
        return diff_ns() / NS2YS;
}

int64_t Timer::diff_ns()
{
        int64_t currentTimestamp = now();
        int64_t difference = currentTimestamp - _timestamp;
        _timestamp = currentTimestamp;
        return difference;
}

void Timer::remember()
{
        _timestamp = now();
}

int64_t Timer::now()
{
        struct timespec result;
        if (clock_gettime(CLOCK_MONOTONIC_RAW, &result) < 0) {
                cout << __FILE__ << "-> " << strerror(errno) << endl;
                return -1;
        }

        int64_t value = result.tv_sec;
        value *= NS; // convert to nanoseconds;
        value += result.tv_nsec;
        
        return value;
}
