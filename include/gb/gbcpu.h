// Source of information from: https://gbdev.io/pandocs/CPU_Registers_and_Flags.html

#pragma once
#include <BaseTsd.h>
#include "gbspace.h"

class gbCpu;

#ifdef LITTLE_ENDIAN
#define REGISTER(name, high, low) \
	union {					\
		struct {			\
			UINT8 low;		\
			UINT8 high;		\
		};					\
		UINT16 name;			\
	}
#else
#define REGISTER(name, high, low) \
	union {					\
		struct {			\
			UINT8 low;		\
			UINT8 high;		\
		};					\
		UINT16 name;			\
	}
#endif

#define BIT(n) (1 << (n))
#define GENMASK(s, b) ((BIT(s + b) - 1) - (BIT(s) - 1))

// interrupt enable flags
#define VBLANK_INTR		BIT(0)
#define LCDSTAT_INTR	BIT(1)
#define TIMER_INTR		BIT(2)
#define SERIAL_INTR		BIT(3)
#define JOYPAD_INTR		BIT(4)

#define ALL_INTR_FLAGS VBLANK_INTR | LCDSTAT_INTR | TIMER_INTR | SERIAL_INTR | JOYPAD_INTR

enum class CpuState {
	Fetch,
	Prefix,
	Working,
	PrefixWork,
	Stop,
	Halt,
};

enum class InterruptEnable {
	EnableDelay,
	Enabled,
	Disabled,
};

typedef void (*Step)(gbCpu*);

struct Instruction {
	const char* name;
	const int bytes;
	const int num_steps;
	const Step steps[6];
};

class gbCpu : public gbSpace {
private:
	bool halt_bug = false;
	const Instruction* currentOp = nullptr;
	int opStep = 0;

	UINT8 requestedIntr() { return interruptsEnabled & interruptFlags; }
	const Instruction* getIntInstruction();
	void printOp(UINT8 opByte);

public:
	gbCpu() {
#ifndef BOOT_ROM
		SP = 0xFFFE;
		PC = 0x100;

#ifdef CGB
		A = 0x11;
		zero = 1;
		BC = 0x0000;
		DE = 0x0800;
		HL = 0x007C;
#else
		A = 0x01;
		zero = 1;
		subtract = 0;
		BC = 0x1300;
		DE = 0xD800;
		HL = 0x4D01;
#endif // CGB

#else
		PC = 0;
#endif // BOOT_ROM
	}

	// Accumulator & Flags
	union {
		struct {
			union {
				UINT8 F;
				struct {
					UINT8 _ : 4;
					UINT8 carry : 1;
					UINT8 half_carry : 1;
					UINT8 subtract : 1;
					UINT8 zero : 1;
				};
			};
			UINT8 A;
		};
		UINT16 AF;
	};

	REGISTER(BC, B, C);
	REGISTER(DE, D, E);
	REGISTER(HL, H, L);
	REGISTER(SP, SP_h, SP_l); // Stack Pointer
	REGISTER(PC, PC_h, PC_l); // Program Counter

	// ALU/Temp registers
	UINT8 high = 0;
	UINT8 low = 0;

	CpuState state{ CpuState::Fetch };
	InterruptEnable interruptMaster{ InterruptEnable::Disabled };
	UINT8 interruptsEnabled{ 0x00 };
	UINT8 interruptFlags{ 0xE1 };

	void step();

	virtual int writeByte(UINT16 addr, UINT8 byte) override;
	virtual int readByte(UINT16 addr) override;
};