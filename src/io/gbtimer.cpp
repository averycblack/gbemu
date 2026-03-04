#include <gb/gbtimer.h>
#include <gb/gb.h>

using namespace gb::timer;
constexpr uint32_t TIMADetectBit[4] { BIT(7), BIT(1), BIT(3), BIT(5) };

constexpr uint8_t REG_TAC_ENABLE = BIT(2);
constexpr uint8_t REG_TAC_FREQ = GENMASK(0, 2);

Timer::Timer() : mDivClk(14), mTimaClk(8) {}

int Timer::writeByte(UINT16 addr, UINT8 val) {
    switch (addr) {
    case 0xFF04:
        mDivClk.mAccum = 0;
        break;
    case 0xFF05:
        if (mTimaState == TimAOverflow) {
            mTimaState = TimAInc;
        }

        if (mTimaState != TimAWrite)
            mTimaClk.mAccum = val;
        break;
    case 0xFF06:
        rTma = val;
        break;
    case 0xFF07:
        // TODO: Remodel tima potentially incrementing?
        // step falling edge detector may be good enough
        rTac = val;
        break;
    default:
        return -1;
    }
    
    return 0;
}

int Timer::readByte(UINT16 addr) {
    switch (addr) {
    case 0xFF04: return (mDivClk.mAccum >> 6) & 0xFF;
    case 0xFF05: return mTimaClk.mAccum & 0xFF;
    case 0xFF06: return rTma;
    case 0xFF07: return rTac;
    default: return -1;
    }
}

gb::utils::Clock &Timer::getDivClk() {
    return mDivClk;
}

void Timer::step() {
    bool timaInc;
    uint32_t divMask = TIMADetectBit[rTac & REG_TAC_FREQ];

    // Increments at 1MHz (M-Cycles)
    mDivClk.increment(1);
    timaInc = mTimaIncDet.sample(mDivClk.mAccum & divMask);

    switch (mTimaState) {
    case TimAInc:
        if (!(rTac & REG_TAC_ENABLE) || !timaInc) {
            break;
        }

        if (mTimaClk.increment(1)) {
            mTimaState = TimAOverflow;
        }
        break;
    case TimAOverflow:
        mTimaClk.mAccum = rTma;
        mTimaState = TimAWrite;
        g_gb->cpu->interruptFlags |= TIMER_INTR;
        break;
    case TimAWrite: 
        mTimaState = TimAInc;
        break;
    }
}