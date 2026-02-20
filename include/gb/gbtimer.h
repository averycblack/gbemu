#pragma once
#include <gb/utils.hpp>
#include <gb/gbspace.h>

#include <stdint.h>

namespace gb::timer {
    class Timer : public gbSpace {
    public:
        Timer();
        virtual int writeByte(UINT16 addr, UINT8 val) override;
        virtual int readByte(UINT16 addr) override;
        void step();
        utils::Clock &getDivClk();
    private:
        enum TIMAOverflow {
            TimAInc,
            TimAOverflow,
            TimAWrite
        };
        
        utils::Clock mDivClk;
        utils::Clock mTimaClk;
        utils::FallingEdgeDetector mTimaIncDet;
        UINT8 rTac = 0;  // Control
        UINT8 rTma;

        TIMAOverflow mTimaState = TimAInc;
    };
};
