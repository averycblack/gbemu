#pragma once
#include <BaseTsd.h>
#include "gbspace.h"

class gbSerial : public gbSpace {
	UINT8 serialData = 0;
	UINT8 serialControl = 0;

public:
	virtual int writeByte(UINT16 addr, UINT8 byte) override;
	virtual int readByte(UINT16 addr) override;
};

