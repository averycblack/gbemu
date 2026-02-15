#pragma once
#include "gbspace.h"

#define WRAM_BANKS_SIZE 0x1000

#define CGB_RAM_BANKS 8

#define HIGH_RAM_SIZE 0xFFFF - 0xFF80

class gbRam : public gbSpace {
	UINT8 highRam[HIGH_RAM_SIZE]{ 0 };
	UINT8 workRam[WRAM_BANKS_SIZE * CGB_RAM_BANKS]{ 0 };

	int wramBank = 0;

public:
	virtual int readByte(UINT16 addr) override;
	virtual int writeByte(UINT16 addr, UINT8 byte) override;
};