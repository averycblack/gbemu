#include <gb/gbsound.h>
#include <BaseTsd.h>
#include <gb/gb.h>
#include <vector>

using namespace gb::sound;

constexpr UINT8 squareWaveDuty[] {
	0b00000001,
	0b10000001,
	0b10000111,
	0b01111110
};

void gbSC1::triggerSound() {
	enabled = true;
	if (nr11Len == 0) 
		nr11Len = 64;

	UINT16 freq = mem[3] | ((mem[4] & 0x7) << 8);
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
	// Check for overflow
	if (newFreq < 0 || newFreq > 2047) {
		enabled = false;
	} else if (write) {
		frequencyShadow = newFreq;
		nr13Freq = newFreq;
	}
}

double gbSC1::step(bool lengthClock, bool volumeClock, bool sweepClock){
	if (lengthClock && enabled && nr14LenEnable) {
		nr11Len--;
		if (nr11Len == 0) {
			enabled = false;
		}
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
		}
	}

	timer -= 1;
	if (timer <= 0) {
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

	if (length == 0)
		length = 64;

	UINT16 freq = mem[3] | ((mem[4] & 0x7) << 8);
	timer = SQUARE_FREQUENCY_PERIOD(freq);
	envelopeTimer = envelopePeriod;
	volume = startingVolume;
}

double gbSC2::step(bool lengthClock, bool volumeClock, bool sweepClock) {
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
		UINT16 freq = mem[3] | ((mem[4] & 0x7) << 8);
		timer = SQUARE_FREQUENCY_PERIOD(freq);
		cycle++;
		cycle %= 8;
	}


	if (!enabled) {
		return 0.0;
	}

	UINT8 duty = squareWaveDuty[dutyCycle];
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
		length = 64 - length;
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
	shiftRegister = 0xFFFF;

	if (length == 0)
		length = 64;

	timer = NOISE_FREQUENCY_PERIOD((divisor + 1) * 2, clockShift);
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

	timer -= 4;
	if (timer <= 0) {
		timer = NOISE_FREQUENCY_PERIOD((divisor + 1) * 2, clockShift);
		UINT16 xorRes = shiftRegister & 1;
		shiftRegister >>= 1;
		xorRes ^= shiftRegister & 1;

		shiftRegister |= xorRes << 14;
		if (width) {
			shiftRegister |= /*(shiftRegister & 0xFFBF) | */(xorRes << 6);
		}
	}

	if (!enabled) {
		return 0.0;
	}

	double effectiveVolume = volume/*/ MAX_VOLUME*/;
	return (!(shiftRegister & 0x1)) * (effectiveVolume / 15.0);
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
	outSpec.format = AUDIO_F32;
	outSpec.channels = 2;
	outSpec.samples = OUTPUT_BUFFER_SIZE;
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
	debugPrint("%x %x\n", addr, byte);
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
	bool envelopeClock = mSweepDet.sample(divApu & BIT(2));

	if (!poweredOn) return;

	double ret1 = sc1.step(lengthClock, envelopeClock, sweepClock);
	double ret2 = sc2.step(lengthClock, envelopeClock, sweepClock);
	double ret3 = sc3.step(lengthClock, envelopeClock, sweepClock);
	double ret4 = sc4.step(lengthClock, envelopeClock, sweepClock);
	
	if (pan & 0x01) sampleLeft += ret1;
	if (pan & 0x02) sampleLeft += ret2;
	if (pan & 0x04) sampleLeft += ret3;
	if (pan & 0x08) sampleLeft += ret4;

	if (pan & 0x10) sampleRight += ret1;
	if (pan & 0x20) sampleRight += ret2;
	if (pan & 0x40) sampleRight += ret3;
	if (pan & 0x80) sampleRight += ret4;

	// leftTotal += double(left * volL * lowPassFilterVals[sample % std::size(lowPassFilterVals) ]);
	// rightTotal += double(right * volR * lowPassFilterVals[sample % std::size(lowPassFilterVals)]);

	sample++;
	int max = OUTPUT_SAMPLES;

	sample %= max;
	if (sample == 0) {
		buf[bufIdx] = (float) sampleLeft * volL / 4.0 / 8.0;
		buf[bufIdx + 1] = (float) sampleRight * volR / 4.0 / 8.0;
		bufIdx += 2;

		// debugPrint("%f %f\n", sampleLeft, sampleRight);

		sampleLeft = 0;
		sampleRight = 0;

		if (bufIdx == SOUND_BUF_SIZE * 2) {
			SDL_QueueAudio(dev, buf.data(), (size_t) SOUND_BUF_SIZE * 2 * sizeof(float));
			bufIdx = 0;
		}
	}
}