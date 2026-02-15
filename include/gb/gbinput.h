#pragma once
#include <BaseTsd.h>
#include "gbspace.h"

#define ACTION_SELECT_MASK 0x20
#define DIRECTION_SELECT_MASK 0x10

enum class gbInputType {
	RIGHT,
	LEFT,
	UP,
	DOWN,
	A,
	B,
	SELECT,
	START
};

class gbInput : public gbSpace {
	union {
		UINT8 directionVal = 0x0F;
		struct {
			UINT8 right : 1;
			UINT8 left : 1;
			UINT8 up : 1;
			UINT8 down : 1;
			UINT8 _ : 4;
		};
	};

	union {
		UINT8 actionVal = 0x0F;
		struct {
			UINT8 a : 1;
			UINT8 b : 1;
			UINT8 select : 1;
			UINT8 start : 1;
			UINT8 _ : 4;
		};
	};

	bool enableActionBtns = false;
	bool enableDirectionBtns = false;

public:
	virtual int writeByte(UINT16 addr, UINT8 val) override;
	virtual int readByte(UINT16 addr) override;

	void setKey(gbInputType type, bool inputDown);
};