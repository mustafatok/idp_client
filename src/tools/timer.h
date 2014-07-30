#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>

class Timer {
public:
        explicit Timer() {
                remember();
        }
        virtual ~Timer() = default;

        int64_t diff_ns();
        int64_t diff_ys();
        int64_t diff_ms();
        void    remember();

private:
        /*!
         *      \return The current timestamp reported by the linux kernel.
         */
        int64_t now();

        int64_t _timestamp = 0;

};

#endif // __TIMER_H
