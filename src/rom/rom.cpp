#include <gb/gbrom.h>
#include <filesystem>
#include <iostream>
#include <fstream>

// No MBC

int romBankSize(int ramSize) {
	switch (ramSize) {
	case 0x2: return 1;
	case 0x3: return 4;
	case 0x4: return 16;
	case 0x5: return 8;
	default: return -1;
	}
}

Rom::Rom(std::filesystem::path path, UINT8* eram, UINT8* rom, size_t romSize)
	: eramPath(path), ram(eram), rom(rom), romSize(romSize) 
{
	hdr = reinterpret_cast<CartHeader*>(rom + 0x100);
	if (hdr->romSize < 0x9) {
		maxRomBanks = (int)std::pow(2, hdr->romSize + 1);
	}

	int banks = romBankSize(hdr->ramSize);
	if (banks == -1) {
		externalRam = false;
	} else {
		maxRamBanks = banks;
	}
}

Rom::~Rom() {
	save();
}

int Rom::readByte(UINT16 addr) {
	if (addr < 0x8000) {
		return rom[addr & 0x7FFF];
	}

	if (externalRam && addr >= 0xA000 && addr < 0xC000) {
		return ram[addr - 0xA000];
	}

	return -1;
}

int Rom::writeByte(UINT16 addr, UINT8 byte) {
	if (addr < 0x8000) {
		// While we do nothing here, it's still within the ROM range
		// and thus still consumes the byte
		return 0;
	}

	if (externalRam && addr >= 0xA000 && addr < 0xC000) {
		ram[addr - 0xA000] = byte;
		return 0;
	}

	return -1;
}

const std::string Rom::getType() {
	if (!externalBatt && !externalRam) return "ROM Only";
	if (externalBatt) return "ROM + RAM + BATT";
	return "ROM + RAM";
}

const CartHeader* Rom::getHeader() {
	return hdr;
}

void Rom::save() {
	if (!externalRam || maxRamBanks < 1) return;

	struct GameSaveHeader hdr {
		1, 0, 0, maxRamBanks * ERAM_SIZE
	};

	std::ofstream of;

	try {
		of.open(eramPath, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
		of.write((char *) &hdr, sizeof(GameSaveHeader));
		of.write((char *) ram, maxRamBanks * ERAM_SIZE);
		of.close();
	} catch (std::ofstream::failure fail) {
		std::cerr << fail.what();
		debugPrint("%s\n", fail.what());
	}
}

bool checksum(UINT8* rom) {
	UINT8 x = 0;
	for (int i = 0x134; i < 0x14D; i++) {
		x += ~rom[i];
	}

	return x == rom[0x014D];
}

bool loadFile(std::filesystem::path& file, UINT8 **buffer, size_t* size) {
	std::ifstream in;

	if (!std::filesystem::exists(file)) {
		return false;
	}

	try {
		in.open(file, std::ios_base::binary | std::ios_base::in);
		in.seekg(0, in.end);
		*size = in.tellg();
		in.seekg(0, in.beg);

		*buffer = new UINT8[*size];
		in.read((char*)(*buffer), *size);

		in.close();
	} catch (std::ifstream::failure e) {
		std::cerr << e.what();
		debugPrint("%s\n", e.what());
		return false;
	}

	return true;
}

bool getEram(std::filesystem::path& eramPath, UINT8 **eram, size_t* size) {
	if (loadFile(eramPath, eram, size)) {
		if (*size < sizeof(GameSaveHeader)) {
			std::string s("Too small save file. Rejecting...");
			displayPopup(s, nullptr);
			delete* eram;
			return false;
		}

		*eram += sizeof(GameSaveHeader);
		return true;
	}
	return false;
}

Rom* createRom(std::filesystem::path& file) {
	UINT8* rom;
	size_t size;

	UINT8* eram = nullptr;
	size_t eramSize = 0;

	if (!loadFile(file, &rom, &size)) {
		return nullptr;
	}

	if (size < 0x200) {
		std::string s("ROM too small");
		displayPopup(s, nullptr);
		return nullptr;
	}

	if (!checksum(rom)) {
		std::string s("Invalid ROM checksum");
		displayPopup(s, nullptr);
		return nullptr;
	}

	CartHeader* hdr = reinterpret_cast<CartHeader*>(rom + 0x100);

	// Try to find file
	std::filesystem::path parent = file.parent_path();
	char title[17]{ 0 };
	if (hdr->cgb.mode & 0x80) {
		memcpy(title, hdr->cgb.title, 11);
	} else {
		memcpy(title, hdr->title, 16);
	}
	std::filesystem::path eramPath = parent / title;
	eramPath += ".sav";
	if (!getEram(eramPath, &eram, &eramSize)) {
		int banks = romBankSize(hdr->ramSize);
		if (banks == -1) banks = 1;
		eram = new UINT8[banks * ERAM_SIZE];
	}

	Rom* romObj = nullptr;
	switch (hdr->cartType) {
	case ROM_ONLY:
		romObj = new Rom(eramPath, eram, rom, size);
		break;
	case MBC1:
	case MBC1_RAM:
	case MBC1_RAM_BATT:
		romObj = new MBC1Rom(eramPath, eram, rom, size);
		break;
	case MBC3:
	case MBC3_RAM:
	case MBC3_RAM_BATT:
	case MBC3_TIMER_BATT:
	case MBC3_TIMER_BATT_RAM:
		romObj = new MBC3Rom(eramPath, eram, rom, size);
		break;
	default: 
		std::string s("Invalid ROM type");
		displayPopup(s, nullptr);
		return nullptr;
	}

	return romObj;
}