#include <gb/gbrom.h>

MBC1Rom::MBC1Rom(std::filesystem::path path, UINT8* eram, UINT8* rom, size_t romSize)
	: Rom(path, eram, rom, romSize) 
{
	if (hdr->cartType == MBC1_RAM_BATT) externalBatt = true;
}

int MBC1Rom::readByte(UINT16 addr) {
	// ROM banks
	if (addr < 0x8000) {
		UINT32 bank = 0;
		
		if (mode == ROMBanking) {
			bank = ramBank << 5;
		}

		// 0000-3FFFF can only access banks 0x00/0x20/0x40/0x60
		// 4000-7FFFF can access banks 0x01-0x1F/0x21-0x3F...
		if (addr >= ROM_BANK_SIZE) {
			bank += romBank;
			addr -= ROM_BANK_SIZE;
		}

		// Inclusive range
		bank %= maxRomBanks + 1;
		UINT32 romAddr = addr + (bank * ROM_BANK_SIZE);
		return rom[romAddr];
	}

	// External RAM
	if (addr >= 0xA000 && addr < 0xC000) {
		addr -= 0xA000;

		if (!externalRam || !ramEnable) {
			return 0xFF;
		}
		
		if (mode == ROMBanking) {
			return rom[addr];
		}
		
		UINT32 bank = ramBank % maxRamBanks;
		return ram[addr + (bank * ERAM_SIZE)];
	}

	return -1;
}

int MBC1Rom::writeByte(UINT16 addr, UINT8 byte) {
	// Ram enable
	if (addr < 0x2000) {
		ramEnable = externalRam && (byte & 0xF) == 0xA;
		return 0;
	}
	
	// Rom banking
	if (addr < 0x4000) {
		// 5 bit register
		int bank = byte & 0x1F;
		
		if (bank == 0) {
			bank = 1;
		}

		romBank = bank;
		return 0;
	} 
	
	// Ram banking (or Rom banking if mode set)
	if (addr < 0x6000) {
		ramBank = byte & 0x3;
		return 0;
	} 
	
	// Banking mode set
	if (addr < 0x8000) {
		mode = (byte & 0x1) ? RAMBanking : ROMBanking;
		return 0;
	} 
	
	// External RAM
	if (addr >= 0xA000 && addr < 0xC000) {
		if (ramEnable) {
			ram[addr - 0xA000] = byte;
		}

		// Still consume byte even if ram is not enabled
		return 0;
	}

	return -1;
}

const std::string MBC1Rom::getType() {
	std::string ret = "MBC1";
	if (externalRam) ret += " + RAM";
	if (externalBatt) ret += " + BATT";
	return ret;
}