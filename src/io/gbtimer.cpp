#include <gb/gbtimer.h>
#include <gb/gb.h>

using namespace gb::timer;

const int freqToBit[4] { 1020, 12, 60, 252 };

int Timer::writeByte(UINT16 addr, UINT8 val) {
	int overflowVal = (freqToBit[tac & 0x3] / 2) + 1;
	bool oldEnable = tac & 0x4;
	bool glitch, newEnable;

	switch (addr) {
	case 0xFF04:
		if (oldEnable && div > overflowVal)
			tima++;

		div = 0;
		break;
	case 0xFF05: 
		if (overflow == Overflow) {
			overflow = None;
		}

		if (overflow != Write)
			tima = val;
		break;
	case 0xFF06: 
		tma = val;
		if (overflow == Write)
			tima = val;
		break;
	case 0xFF07:
		newEnable = val & 0x4;

		glitch = !newEnable || (div <= (freqToBit[val & 0x3] / 2) + 1);
		if (oldEnable && div > overflowVal && glitch)
			tima++;

		tac = val; 
		break;
	default: return -1;
	}
	
	return 0;
}

int Timer::readByte(UINT16 addr) {
	switch (addr) {
	case 0xFF04: return div >> 8;
	case 0xFF05: return tima;
	case 0xFF06: return tma;
	case 0xFF07: return tac;
	default: return -1;
	}
}

void Timer::step() {
	int overflowMask = freqToBit[tac & 0x3];

	// Detect falling edge to increment tima
	div += 4;

	switch (overflow) {
	case Write: 
		overflow = None;
		break;
	case Overflow:
		tima = tma;
		overflow = Write;
		g_gb->cpu->interruptFlags |= TIMER_INTR;
		break;
	}

	if ((tac & 0x4) == 0 || div & overflowMask) {
		return;
	}

	tima++;
	if (tima == 0) {
		overflow = Overflow;
	}
}