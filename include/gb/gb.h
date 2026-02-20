#pragma once
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <gb/gbram.h>
#include <gb/gbrom.h>
#include <gb/gbcpu.h>
#include <gb/gbgpu.h>
#include <gb/gbinput.h>
#include <gb/gbtimer.h>
#include <gb/gbserial.h>
#include <gb/gbsound.h>
#include <vector>
#include <string>

// #ifdef BIG_ENDIAN
// #error "Big Endian not supported"
// #endif

// #if !defined BIG_ENDIAN && !defined LITTLE_ENDIAN
// #error "Endianness not given"
// #endif

#define MAX_SIZE 0xFFFF

#define GB_HZ 4194304
#define SERIAL_HZ 8192
#define SERIAL_DELAY GB_HZ / SERIAL_HZ

#pragma warning( push )
#pragma warning( disable: 26495 )

#ifdef _DEBUG
inline void debugPrint(const char* fmt...) {
	char str[1024];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(str, sizeof(str), fmt, args);
	va_end(args);
	OutputDebugStringA(str);
}
#else
inline void debugPrint(const char* fmt...) {}
#endif

#pragma warning( pop )

inline UINT64 g_cycles = 0;
class Rom;

class GameboyEmu {
public:
	GameboyEmu(Rom* rom) : rom(rom) {
		cpu = new gbCpu();
		gpu = new gbGpu();
		ram = new gbRam();
		input = new gbInput();
		serial = new gbSerial();
		timer = new gb::timer::Timer();
		sound = new gb::sound::APU(*timer);
#ifdef BOOT_ROM
		bootRomEnable = true;
#endif
	}

	~GameboyEmu() {
		delete rom;
		delete sound;
	}

	void step();

	// Memory Read/Writes
	UINT8 readByte(UINT16 addr);
	UINT16 readShort(UINT16 addr);
	void writeByte(UINT16 addr, UINT8 val);

	// Stack
	void pushStackByte(UINT8 val);
	UINT8 popStackByte();

	gbCpu* cpu;
	gbGpu* gpu;
	gbRam* ram;
	gbInput* input;
	gbSerial* serial;
	gb::sound::APU* sound;
	gb::timer::Timer* timer;
private:
	bool bootRomEnable{ false };
	Rom* rom;
};

inline GameboyEmu* g_gb;

HRESULT openFileMenu(PWSTR*);
void displayPopup(const std::string& text, HWND hWnd);
