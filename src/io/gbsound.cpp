#include <gb/gbsound.h>
#include <BaseTsd.h>
#include <gb/gb.h>
#include <vector>

using namespace gb::sound;

static const double lowPassFilterVals[]{
	-0.000071,
	0.000030,
	0.000434,
	0.001340,
	0.002971,
	0.005539,
	0.009210,
	0.014063,
	0.020059,
	0.027019,
	0.034622,
	0.042422,
	0.049889,
	0.056458,
	0.061599,
	0.064875,
	0.066000,
	0.064875,
	0.061599,
	0.056458,
	0.049889,
	0.042422,
	0.034622,
	0.027019,
	0.020059,
	0.014063,
	0.009210,
	0.005539,
	0.002971,
	0.001340,
	0.000434,
	0.000030,
	-0.000071
};

static const UINT8 squareWaveDuty[] {
	0b00000001,
	0b10000001,
	0b10000111,
	0b01111110
};

void gbSC1::triggerSound() {
	enabled = true;
	debugPrint("Channel 1 Triggered!\n");
	if (nr11Len == 0) 
		nr11Len = 64;

	UINT16 freq = nr13FreqLow | (nr14FreqHigh << 8);
	timer = SQUARE_FREQUENCY_PERIOD(freq);
	envelopeTimer = nr12EnvPeriod;
	if (nr12EnvPeriod == 0)
		envelopeTimer = 8;
	volume = nr12Vol;

	// Sweep specifics
	frequencyShadow = freq;
	sweepTimer = nr10SwpPace;
	sweepEnabled = nr10SwpPace != 0 || nr10SwpStep != 0;
	if (nr10SwpStep != 0) {
		shiftFreqCheck(false);
	}
}

void gbSC1::shiftFreqCheck(bool write) {
	int shifted = frequencyShadow >> nr10SwpStep;
	int newFreq = frequencyShadow + (nr10SwpDir ? -shifted : shifted);
	if (nr10SwpDir && shifted > frequencyShadow)
		newFreq = 0;
	// Check for overflow
	if (newFreq > 0x7FF) {
		enabled = false;
	} else if (write) {
		frequencyShadow = newFreq;
		nr13FreqLow = newFreq & 0xFF;
		nr14FreqHigh = (newFreq >> 8) & 0x7;
	}
}

double gbSC1::step(bool lengthClock, bool volumeClock, bool sweepClock){
	if (lengthClock && enabled && nr14LenEnable) {
		nr11Len--;
		enabled = nr11Len != 0;
	}

	if (volumeClock && envelopeTimer > 0) {
		envelopeTimer--;
		// Volume Envelope stops when at min or max
		if (envelopeTimer == 0 && nr12EnvPeriod != 0) {
			int tempVol = volume + (nr12EnvDir ? 1 : -1);
			if (tempVol >= 0 && tempVol <= 15) {
				volume = tempVol;
				envelopeTimer = nr12EnvPeriod;
				if (nr12EnvPeriod == 0)
					envelopeTimer = 8;
			}
		}
	}

	if (sweepClock && sweepTimer > 0) {
		sweepTimer--;
		if (sweepTimer == 0 && sweepEnabled && nr10SwpPace != 0) {
			sweepTimer = nr10SwpPace;
			shiftFreqCheck(true);
			shiftFreqCheck(false);
		}
	}

	timer -= 1;
	if (timer <= 0) {
		// 1MHz timer, 11 bit register
		timer = SQUARE_FREQUENCY_PERIOD(frequencyShadow);
		cycle++;
		cycle %= 8;
	}
	
	if (!enabled) {
		return 0.0;
	}

	UINT8 duty = squareWaveDuty[nr11DutyCycle];
	double effectiveVolume = volume/*/ MAX_VOLUME*/;
	return double((duty >> cycle) & 0x1) * effectiveVolume / 15.0;
}

int gbSC1::writeByte(UINT16 addr, UINT8 byte) {
	if (addr < 0xFF10 || addr > 0xFF14) return -1;

	mem[addr - 0xFF10] = byte;
	if (addr == 0xFF14 && byte & 0x80) {
		triggerSound();
	}

	if (addr == 0xFF11) {
		nr11Len = 64 - nr11Len;
	}

	return 0;
}

#define DUTYCYCLE_MASK 0xC0
#define COUNTER_MASK 0x40

int gbSC1::readByte(UINT16 addr) {
	switch (addr) {
	case 0xFF10: return mem[0];
	case 0xFF11: return (mem[1] & DUTYCYCLE_MASK) | UINT8(~DUTYCYCLE_MASK);
	case 0xFF12: return mem[2];
	case 0xFF13: return 0xFF;
	case 0xFF14: return (mem[4] & COUNTER_MASK) | UINT8(~COUNTER_MASK);
	}

	return -1;
}

// ******************* Sound Channel 2 *******************

void gbSC2::triggerSound() {
	enabled = true;
	debugPrint("Channel 2 Triggered!\n");

	if (nr21Len == 0)
		nr21Len = 64;

	UINT16 freq = nr23FreqLow | (nr24FreqHigh << 8);
	timer = SQUARE_FREQUENCY_PERIOD(freq);
	envelopeTimer = nr22EnvPeriod;
	volume = nr22Vol;
}

double gbSC2::step(bool lengthClock, bool volumeClock, bool sweepClock) {
	if (lengthClock && enabled && nr24LenEnable) {
		nr21Len--;
		enabled = nr21Len != 0;
	}

	if (volumeClock && envelopeTimer > 0) {
		envelopeTimer--;
		// Volume Envelope stops when at min or max
		if (envelopeTimer == 0 && nr22EnvPeriod != 0) {
			int tempVol = volume + (nr22EnvDir ? 1 : -1);
			if (tempVol >= 0 && tempVol <= 15) {
				volume = tempVol;
				envelopeTimer = nr22EnvPeriod;
			}
		}
	}

	timer -= 1;
	if (timer <= 0) {
		// 1MHz timer, 11 bit register
		UINT16 freq = nr23FreqLow | (nr24FreqHigh << 8);
		timer = SQUARE_FREQUENCY_PERIOD(freq);
		cycle++;
		cycle %= 8;
	}


	if (!enabled) {
		return 0.0;
	}

	UINT8 duty = squareWaveDuty[nr21DutyCycle];
	double effectiveVolume = volume/*/ MAX_VOLUME*/;
	return ((duty >> cycle) & 0x1) * effectiveVolume / 15.0;
}

int gbSC2::writeByte(UINT16 addr, UINT8 byte) {
	if (addr < 0xFF15 || addr > 0xFF19) return -1;

	mem[addr - 0xFF15] = byte;
	if (addr == 0xFF19 && byte & 0x80) {
		triggerSound();
	}

	if (addr == 0xFF16) {
		nr21Len = 64 - nr21Len;
	}

	return 0;
}

int gbSC2::readByte(UINT16 addr) {
	switch (addr) {
	case 0xFF15: return 0xFF;
	case 0xFF16: return (mem[1] & DUTYCYCLE_MASK) | UINT8(~DUTYCYCLE_MASK);
	case 0xFF17: return mem[2];
	case 0xFF18: return 0xFF;
	case 0xFF19: return (mem[4] & COUNTER_MASK) | UINT8(~COUNTER_MASK);
	}

	return -1;
}

// ******************* Sound Channel 3 *******************

void gbSC3::triggerSound() {
	enabled = true;
	debugPrint("Channel 3 Triggered!\n");

	// During the clock, length is decremented first before checking for 0.
	// This will underflow to 255, acting like it was 256 initially.
	if (length == 0)
		length = 0;

	sample = 0;

	UINT16 freq = mem[3] | ((mem[4] & 0x7) << 8);
	timer = WAVE_FREQUENCY_PERIOD(freq);
}	

double gbSC3::step(bool lengthClock, bool volumeClock, bool sweepClock) {
	if (lengthClock && enabled && counter) {
		length--;
		if (length == 0) {
			enabled = false;
		}
	}

	timer -= 2;
	if (timer <= 0) {
		UINT16 freq = mem[3] | ((mem[4] & 0x7) << 8);
		// Clocked at 2MHz
		timer += WAVE_FREQUENCY_PERIOD(freq);
		sample++;
		sample %= 32;
	}


	if (!enabled || !dacPower) {
		return 0.0;
	}

	// Each byte is two samples, upper nibble first
	UINT8 duty = waveTable[sample / 2];
	if (sample % 2 == 0) {
		duty >>= 4;
	}

	duty &= 0x0F;

	return (duty >> volumeCodes[volume]) / 16.0;
}

int gbSC3::writeByte(UINT16 addr, UINT8 byte) {
	if (addr >= 0xFF1A && addr < 0xFF1F) {
		mem[addr - 0xFF1A] = byte;
		if (addr == 0xFF1E && byte & 0x80) {
			triggerSound();
		}

		if (addr == 0xFF1B) {
			length = 256 - length;
		}


		return 0;
	}

	if (addr >= 0xFF30 && addr < 0xFF40) {
		waveTable[addr - 0xFF30] = byte;
		return 0;
	}

	return -1;
}

int gbSC3::readByte(UINT16 addr) {
	switch (addr) {
	case 0xFF1A: return (mem[0] & 0x80) | 0x7F;
	case 0xFF16: return mem[1];
	case 0xFF17: return (mem[2] & 0x60) | 0x9F;
	case 0xFF18: return 0xFF;
	case 0xFF19: return (mem[4] & COUNTER_MASK) | UINT8(~COUNTER_MASK);
	}

	if (addr >= 0xFF30 && addr < 0xFF40) {
		return waveTable[addr - 0xFF30];
	}

	return -1;
}

// ******************* Sound Channel 4 *******************

void gbSC4::triggerSound() {
	enabled = true;
	debugPrint("Channel 4 Triggered!\n");
	shiftRegister = 0x0;

	if (length == 0)
		length = 64;

	// Clocked at 262144 / (divider * 2^shift)
	// = 1MHz / (4 * divider * 2^shift)
	// = 1MHz / (divider * 2^(shift + 2))
	timer = divisor << clockShift + 2;
	envelopeTimer = envelopePeriod;
	volume = startingVolume;
}

double gbSC4::step(bool lengthClock, bool volumeClock, bool sweepClock) {
	if (lengthClock && enabled && counter) {
		length--; 
		if (length == 0) {
			enabled = false;
		}
	}

	if (volumeClock && envelopeTimer > 0) {
		envelopeTimer--;
		// Volume Envelope stops when at min or max
		if (envelopeTimer == 0 && envelopePeriod != 0) {
			int tempVol = volume + (envelopeAdd ? 1 : -1);
			if (tempVol >= 0 && tempVol <= 15) {
				volume = tempVol;
				envelopeTimer = envelopePeriod;
			}
		}
	}

	timer -= 1;
	if (timer <= 0) {
		// Clocked at 262144 / (divider * 2^shift)
		// = 1MHz / (4 * divider * 2^shift)
		// = 1MHz / (divider * 2^(shift + 2))
		timer = divisor << clockShift + 2;
		UINT16 xorRes = (shiftRegister & 1) ^ ((shiftRegister >> 1) & 1);
		xorRes ^= 1; // invert
		
		shiftRegister &= ~BIT(15);
		shiftRegister |= xorRes << 15;
		if (width) {
			shiftRegister &= ~BIT(7);
			shiftRegister |= xorRes << 7;
		}

		shiftRegister >>= 1;
	}

	if (!enabled) {
		return 0.0;
	}

	double effectiveVolume = volume/*/ MAX_VOLUME*/;
	return (shiftRegister & 0x1) * (effectiveVolume / 15.0);
}

int gbSC4::writeByte(UINT16 addr, UINT8 byte) {
	if (addr < 0xFF1F || addr > 0xFF23) return -1;

	mem[addr - 0xFF1F] = byte;
	if (addr == 0xFF23 && byte & 0x80) {
		triggerSound();
	}

	if (addr == 0xFF20) {
		length = 64 - length;
	}

	return 0;
}

int gbSC4::readByte(UINT16 addr) {
	switch (addr) {
	case 0xFF1A: return 0xFF;
	case 0xFF20: return 0xFF;
	case 0xFF21: return mem[2];
	case 0xFF22: return mem[3];
	case 0xFF23: return (mem[4] & COUNTER_MASK) | UINT8(~COUNTER_MASK);
	}

	return -1;
}


// ******************* Sound Controller *******************

APU::APU(gb::timer::Timer &timer) : mTimer(timer), mDivApuClk(8) {
	SDL_AudioSpec outSpec{ 0 };
	SDL_AudioSpec gotSpec{ 0 };

	outSpec.freq = OUTPUT_FREQ;
	outSpec.format = AUDIO_F32SYS;
	outSpec.channels = 2;
	outSpec.samples = SOUND_BUF_SIZE;
	outSpec.callback = nullptr;

	dev = SDL_OpenAudioDevice(NULL, 0, &outSpec, &gotSpec, 0);
	if (dev == 0) {
		debugPrint("%s!\n", SDL_GetError());
		exit(1);
	}

	SDL_PauseAudioDevice(dev, 0);
}

int APU::writeByte(UINT16 addr, UINT8 byte) {
	UINT8 on;
	// debugPrint("%x %x\n", addr, byte);
	if (sc1.writeByte(addr, byte) == 0) return 0;
	if (sc2.writeByte(addr, byte) == 0) return 0;
	if (sc3.writeByte(addr, byte) == 0) return 0;
	if (sc4.writeByte(addr, byte) == 0) return 0;

	switch (addr) {
	case 0xFF24: mem[0] = byte; break;
	case 0xFF25: mem[1] = byte; break;
	case 0xFF26: 
		on = (byte & 0x80) >> 7;
		if (!on) {
			memset(sc1.mem, 0, sizeof(sc1.mem));
			memset(sc2.mem, 0, sizeof(sc2.mem));
			memset(sc3.mem, 0, sizeof(sc3.mem));
			memset(sc4.mem, 0, sizeof(sc4.mem));
		} else if (!poweredOn) {
			mDivApuClk.mAccum = 0;
			sc1.cycle = 0;
		}

		poweredOn = on;
		break;
	default: return -1;
	}


	return 0;
}

int APU::readByte(UINT16 addr) {
	int ret = sc1.readByte(addr);
	if (ret != -1) return ret;

	ret = sc2.readByte(addr);
	if (ret != -1) return ret;

	ret = sc3.readByte(addr);
	if (ret != -1) return ret;

	ret = sc4.readByte(addr);
	if (ret != -1) return ret;

	switch (addr) {
	case 0xFF24: return mem[0];
	case 0xFF25: return mem[1];
	case 0xFF26: 
		UINT8 enabled = 0;
		enabled |= sc1.enabled << 0;
		enabled |= sc2.enabled << 1;
		enabled |= sc3.enabled << 2;
		enabled |= sc4.enabled << 3;
		return (mem[2] & 0x80) | enabled | 0x70;
	}

	return -1;
}

void APU::step() {
	auto clk = mTimer.getDivClk();
	// BIT 11 in double speed mode
	if (mDivApuDet.sample(clk.mAccum & BIT(10))) {
		(void) mDivApuClk.increment(1);
	}

	uint32_t divApu = mDivApuClk.mAccum;
	bool lengthClock = mLengthDet.sample(divApu & BIT(0));
	bool sweepClock = mSweepDet.sample(divApu & BIT(1));
	bool envelopeClock = mEnvelopeDet.sample(divApu & BIT(2));

	if (!poweredOn) return;

	double ret1 = sc1.step(lengthClock, envelopeClock, sweepClock);
	double ret2 = sc2.step(lengthClock, envelopeClock, sweepClock);
	double ret3 = sc3.step(lengthClock, envelopeClock, sweepClock);
	double ret4 = sc4.step(lengthClock, envelopeClock, sweepClock);
	
	double left = 0, right = 0;

	if (pan & 0x01) left += ret1;
	if (pan & 0x02) left += ret2;
	if (pan & 0x04) left += ret3;
	if (pan & 0x08) left += ret4;

	if (pan & 0x10) right += ret1;
	if (pan & 0x20) right += ret2;
	if (pan & 0x40) right += ret3;
	if (pan & 0x80) right += ret4;

	sampleLeft += double(left * volL * lowPassFilterVals[sample]);
	sampleRight += double(right * volR * lowPassFilterVals[sample]);

	sample++;
	sample %= OUTPUT_SAMPLES;
	if (sample == 0) {
		constexpr double NumChannels = 4.0;
		constexpr double MaxVol = 8.0;
		buf[bufIdx] = (float) sampleLeft * volL / (MaxVol * NumChannels * 8);
		buf[bufIdx + 1] = (float) sampleRight * volR / (MaxVol * NumChannels * 8);
		bufIdx += 2;

		sampleLeft = 0;
		sampleRight = 0;

		if (bufIdx == SOUND_BUF_SIZE * 2) {
			SDL_QueueAudio(dev, buf.data(), (uint32_t) SOUND_BUF_SIZE * 2 * sizeof(float));
			bufIdx = 0;
		}
	}
}