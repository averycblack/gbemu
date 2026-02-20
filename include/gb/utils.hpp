#pragma once

#include <stdint.h>

namespace gb::utils {
    /*
    * Clock with automatic rollover/reload.
    * A clock with an InitVal of 0 will behave as if it rolled over.
    */
    class Clock {
    public:
        Clock(size_t clkBits);
        
        /*
         * Increment clock by stepsize
         * Return: True on register overflow 
         */
        bool increment(uint32_t stepSize);

        size_t mInitVal { 0 };
        uint32_t mAccum { 0 };
    private:
        uint32_t mMaxVal;
    };

    /*
    * Simple falling edge detector that detects a 1 -> 0 transition.
    */
    class FallingEdgeDetector {
    public:
        bool sample(bool curVal);
    private:
        bool mLastVal {false};
    };
};
