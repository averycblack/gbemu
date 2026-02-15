#include <gb/gbserial.h>
#include <gb/gb.h>

int gbSerial::writeByte(UINT16 addr, UINT8 byte) {
	switch (addr) {
	case 0xFF01: serialData = byte; break;
	case 0xFF02:
		serialControl = byte;
		break;
	default: return -1;
	}

	return 0;
}

int gbSerial::readByte(UINT16 addr) {
	switch (addr) {
	case 0xFF01: return serialData;
	case 0xFF02: return serialControl;
	default: return -1;
	}
}