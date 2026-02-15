#include <gb/gbrom.h>

MBC3Rom::MBC3Rom(std::filesystem::path path, UINT8* eram, UINT8* rom, size_t romSize)
	: Rom(path, eram, rom, romSize)
{
	switch (hdr->cartType) {
	case MBC3_TIMER_BATT:
	case MBC3_TIMER_BATT_RAM:
		rtcEpoch = std::chrono::system_clock::now();
		externalTimer = true;
		[[fallthrough]];
	case MBC3_RAM_BATT:
		externalBatt = true;
		break;
	}
}

int MBC3Rom::readByte(UINT16 addr) {
	// Lower ROM Bank
	if (addr < 0x4000) {
		return rom[addr];
	} 
	
	// Upper ROM bank (Switchable)
	if (addr < 0x8000) {
		addr -= ROM_BANK_SIZE;
		int bank = romBank % maxRomBanks;
		return rom[addr + (bank * ROM_BANK_SIZE)];
	} 
	
	// External RAM
	if (addr >= 0xA000 && addr <= 0xC000) {
		addr -= 0xA000;

		if (!externalRam || !ramEnable) {
			return 0xFF;
		}

		if (externalTimer && ramBank > 0x8 && ramBank < 0x0D) {
			debugPrint("Reading RTC\n");
			const auto now = std::chrono::system_clock::now();
			const auto epoch = clockLatched == Latched ? latchedTime : rtcEpoch;
			const auto timeSinceEpoch = rtcEpoch - now;
			switch (ramBank) {
			case 0x8: 
				const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeSinceEpoch);
				return seconds.count() % 60;
			case 0x9:
				const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(timeSinceEpoch);
				return minutes.count() % 60;
			case 0xA:
				const auto hours = std::chrono::duration_cast<std::chrono::hours>(timeSinceEpoch);
				return hours.count() % 24;
			case 0xB:
				// Lower 8 bits of day
				const auto days = std::chrono::duration_cast<std::chrono::days>(timeSinceEpoch);
				return days.count() % 0xFF;
			//case 0xC:
			//	const auto days = std::chrono::duration_cast<std::chrono::days>(timeSinceEpoch);
			//	return (days.count() >> 8) & 1;
			default:
				return 0x0;
			}

		} else {
			if (ramBank >= maxRamBanks) return 0xFF;
			int bank = ramBank % maxRamBanks;
			return ram[addr + (bank * ERAM_SIZE)];
		}

	}

	return -1;
}

int MBC3Rom::writeByte(UINT16 addr, UINT8 byte) {
	// External Ram Enable
	if (addr < 0x2000) {
		ramEnable = externalRam && (byte & 0xF) == 0xA;
		return 0;
	}
	
	// ROM Bank Select
	if (addr < 0x4000) {
		romBank = byte & 0x7F;
		if (romBank == 0) romBank = 1;
		return 0;
	}
	
	// RAM/RTC Bank Select
	if (addr < 0x6000) {
		debugPrint("Select 0x%x\n", byte);
		ramBank = byte;
		return 0;
	}
	
	// Real time clock latching
	if (addr < 0x8000) {
		switch (clockLatched) {
		case Unlatched: 
			if (byte == 0)
				clockLatched = ByteOne;
			break;
		case ByteOne:
			if (byte == 1) {
				clockLatched = Latched;
				latchedTime = std::chrono::system_clock::now();
			} else {
				clockLatched = Unlatched;
			}
			break;
		}
		return 0;
	}

	// External RAM
	if (addr >= 0xA000 && addr <= 0xC000) {
		if (ramEnable && ramBank < maxRamBanks) {
			int bank = ramBank % maxRamBanks;
			ram[addr - 0xA000 + (bank * ERAM_SIZE)] = byte;
		}

		// Always consume even if RAM is not enabled
		return 0;
	}

	return -1;
}

const std::string MBC3Rom::getType() {
	std::string ret = "MBC3";
	if (externalTimer) ret += " + Timer";
	if (externalRam) ret += " + RAM";
	if (externalBatt) ret += " + BATT";
	return ret;
}