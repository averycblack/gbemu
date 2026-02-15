#pragma once
#include <BaseTsd.h>

class gbSpace {
public:
	virtual int writeByte(UINT16 addr, UINT8 byte) = 0;
	virtual int readByte(UINT16 addr) = 0;
};

