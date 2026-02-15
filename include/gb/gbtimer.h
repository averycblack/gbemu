#pragma once
#include <BaseTsd.h>
#include "gbspace.h"

namespace gb::timer {
    class Clock {
    public:
        Clock(size_t clkBits);
        
        bool increment(uint32_t stepSize);

        size_t mInitVal { 0 };
        uint32_t mAccum { 0 };
    private:
        uint32_t maxVal;
    };

    class FallingEdgeDetector {
    public:
        bool sample(bool curVal);
    private:
        bool mLastVal {false};
    };

    class TriggeredClock {
    public:
        bool 
    };

    class Timer : public gbSpace {
    public:
        virtual int writeByte(UINT16 addr, UINT8 val) override;
        virtual int readByte(UINT16 addr) override;
        void step();
        Clock &getDivClk();
    private:
        enum TIMAOverflow {
            None,
            Overflow,
            Write
        };
        
        UINT16 div = 0;  // Always counts up
        UINT8 tima = 0;
        UINT8 tma = 0;  // Initial tima value
        UINT8 tac = 0;  // Control

        TIMAOverflow overflow = None;
    };
};
