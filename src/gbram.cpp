#include <gb/gbram.h>
#include <gb/gb.h>

int gbRam::readByte(UINT16 addr) {
	// Echoed RAM
	if (addr >= 0xE000 && addr < 0xFE00) {
		addr -= 0x2000;
	}

	// Work RAM
	if (addr >= 0xC000 && addr < 0xE000) {
		addr -= 0xC000;

		if (addr < 0x1000) {
			return workRam[addr];
		}

		return workRam[addr  + (wramBank * WRAM_BANKS_SIZE)];
	}

	// High RAM
	if (addr >= 0xFF80 && addr < 0xFFFF) {
		return highRam[addr - 0xFF80];
	}

	// Work RAM Bank Select
	if (addr == 0xFF70) {
#ifdef CGB
		return wramByte;
#else
		return 0xFF;
#endif
	}

	return -1;
}

int gbRam::writeByte(UINT16 addr, UINT8 val) {
	// Echoed RAM
	if (addr >= 0xE000 && addr < 0xFE00) {
		addr -= 0x2000;
	}

	// Work RAM
	if (addr >= 0xC000 && addr < 0xE000) {
		addr -= 0xC000;
		if (addr < 0x1000) {
			workRam[addr] = val;
		} else {
			workRam[addr + (WRAM_BANKS_SIZE * wramBank)] = val;
		}

		return 0;
	}

	// High RAM
	if (addr >= 0xFF80 && addr < 0xFFFF) {
		highRam[addr - 0xFF80] = val;
		return 0;
	}

	// Work RAM Bank Select
	if (addr == 0xFF70) {
#ifdef CGB
		wramBank = val & 7;
		if (wramBank == 0) {
			wramBank = 1;
		}
#endif
		return 0;
	}

	return -1;
}