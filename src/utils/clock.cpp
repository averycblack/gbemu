#include <gb/utils.hpp>

using namespace gb::utils;

Clock::Clock(size_t clkBits) : mMaxVal(1 << clkBits) {}

bool Clock::increment(uint32_t stepSize) {
    uint32_t overflow;
    mAccum += stepSize;

    if (mAccum < mMaxVal) {
        return false;
    }

    overflow = mAccum % mMaxVal;
    mAccum = mInitVal + overflow;
    return true;
}

bool FallingEdgeDetector::sample(bool curVal) {
    bool retVal = !curVal && mLastVal;
    mLastVal = curVal;
    return retVal;
}