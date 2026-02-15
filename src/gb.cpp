#include <gb/gb.h>
#include <gb/gbgpu.h>
#include <gb/gbtimer.h>
#include <tchar.h>
#include <Windows.h>
#include <SDL.h>

#ifdef BOOT_ROM
#include "bootRom.h"
#endif

// Stack decrement first, increments after

void GameboyEmu::pushStackByte(UINT8 val) {
	writeByte(--cpu->SP, val);
}

UINT8 GameboyEmu::popStackByte() {
	return readByte(cpu->SP++);
}

UINT8 GameboyEmu::readByte(UINT16 addr) {
	int ret;

#ifdef BOOT_ROM
	if (bootRomEnable && addr < 0x100)
		return bootRom[addr];
#endif

	ret = rom->readByte(addr);
	if (ret != -1) return ret;

	ret = gpu->readByte(addr);
	if (ret != -1) return ret;

	ret = ram->readByte(addr);
	if (ret != -1) return ret;

	// TODO: OAM Corruption
	if (addr >= 0xFEA0 && addr < 0xFF00) return 0xFF;

	ret = input->readByte(addr);
	if (ret != -1) return ret;

	ret = serial->readByte(addr);
	if (ret != -1) return ret;

	ret = timer->readByte(addr);
	if (ret != -1) return ret;

	ret = sound->readByte(addr);
	if (ret != -1) return ret;

	ret = cpu->readByte(addr);
	if (ret != -1) return ret;

	return 0xFF;
}

UINT16 GameboyEmu::readShort(UINT16 addr) {
	return (readByte(addr + 1) << 8) + readByte(addr);
}

void GameboyEmu::writeByte(UINT16 addr, UINT8 byte) {
	int ret;

	ret = rom->writeByte(addr, byte);
	if (ret != -1) return;

	ret = gpu->writeByte(addr, byte);
	if (ret != -1) return;

	ret = ram->writeByte(addr, byte);
	if (ret != -1) return;

	// TODO: OAM Corruption
	if (addr >= 0xFEA0 && addr < 0xFF00) return;

	ret = input->writeByte(addr, byte);
	if (ret != -1) return;

	ret = serial->writeByte(addr, byte);
	if (ret != -1) return;

	ret = timer->writeByte(addr, byte);
	if (ret != -1) return;

	ret = sound->writeByte(addr, byte);
	if (ret != -1) return;

	ret = cpu->writeByte(addr, byte);
	if (ret != -1) return;

	// BANK register
	if (addr == 0xFF50) {
		bootRomEnable = false;
	}
}