#include <gb/gbsound.h>
#include <BaseTsd.h>
#include <gb/gb.h>
#include <vector>

constexpr UINT8 squareWaveDuty[] {
	0b00000001,
	0b10000001,
	0b10000111,
	0b01111110
};

void gbSC1::triggerSound() {
	enabled = true;
	if (length == 0) 
		length = 64;

	UINT16 freq = mem[3] | ((mem[4] & 0x7) << 8);
	timer = SQUARE_FREQUENCY_PERIOD(freq);
	envelopeTimer = envelopePeriod;
	volume = startingVolume;

	// Sweep specifics
	frequencyShadow = freq;
	sweepTimer = sweepPeriod;
	sweepEnabled = sweepPeriod != 0 || sweepShift != 0;
	if (sweepShift != 0)
		shiftFrequency(true);
}

void gbSC1::shiftFrequency(bool write) {
	int shifted = frequencyShadow >> sweepShift;
	int newFreq = frequencyShadow + (sweepNegate ? -shifted : shifted);
	// Check for overflow
	if (newFreq < 0 || newFreq > 2047) {
		enabled = false;
	} else if (write && sweepShift != 0) {
		frequencyShadow = newFreq;
		frequency = newFreq;
	}
}

double gbSC1::step(bool lengthClock, bool volumeClock, bool sweepClock){
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

	if (sweepClock && sweepPeriod > 0) {
		sweepTimer--;
		if (sweepTimer == 0 && sweepEnabled && sweepPeriod != 0) {
			sweepTimer = sweepPeriod;
			shiftFrequency(true);
			shiftFrequency(false);
		}
	}

	timer -= 4;
	if (timer <= 0) {
		timer = SQUARE_FREQUENCY_PERIOD(frequencyShadow);
		cycle++;
		cycle %= 8;
	}

	
	if (!enabled) {
		return 0.0;
	}

	UINT8 duty = squareWaveDuty[dutyCycle];
	double effectiveVolume = volume/*/ MAX_VOLUME*/;
	return ((duty >> cycle) & 0x1) * effectiveVolume;
}

int gbSC1::writeByte(UINT16 addr, UINT8 byte) {
	if (addr < 0xFF10 || addr > 0xFF14) return -1;

	mem[addr - 0xFF10] = byte;
	if (addr == 0xFF14 && byte & 0x80) {
		triggerSound();
	}

	if (addr == 0xFF11) {
		length = 64 - length;
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

	timer -= 4;
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
	return ((duty >> cycle) & 0x1) * effectiveVolume;
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

	timer -= 4;
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

	return (duty >> volumeCodes[volume]);
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

	timer = NOISE_FREQUENCY_PERIOD(divisorCodes[divisor], clockShift);
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
		timer = NOISE_FREQUENCY_PERIOD(divisorCodes[divisor], clockShift);
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
	return (!(shiftRegister & 0x1)) * effectiveVolume;
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

gbSound::gbSound() {
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

int gbSound::writeByte(UINT16 addr, UINT8 byte) {
	UINT8 on;
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
			frameSequencer = 0;
			sc1.cycle = 0;
		}

		poweredOn = on;
		break;
	default: return -1;
	}


	return 0;
}

int gbSound::readByte(UINT16 addr) {
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

void gbSound::step() {
	bool lengthClock = false;
	bool sweepClock = false;
	bool envelopeClock = false;

	frameCounter -= 4;
	if (frameCounter <= 0) {
		frameCounter = FRAME_SEQUENCE_PERIOD;
		if ((frameSequencer & 1) == 0) {
			lengthClock = true;
		}

		if ((frameSequencer & 3) == 2) {
			sweepClock = true;
		}

		if (frameSequencer == 7) {
			envelopeClock = true;
		}

		frameSequencer++;
		frameSequencer %= 8;
	}

	if (!poweredOn) return;

	double left = 0;
	double right = 0;

	double ret1 = sc1.step(lengthClock, envelopeClock, sweepClock);
	double ret2 = sc2.step(lengthClock, envelopeClock, sweepClock);
	double ret3 = sc3.step(lengthClock, envelopeClock, sweepClock);
	double ret4 = sc4.step(lengthClock, envelopeClock, sweepClock);
	
	if (pan & 0x01) left += ret1;
	if (pan & 0x02) left += ret2;
	if (pan & 0x04) left += ret3;
	if (pan & 0x08) left += ret4;

	if (pan & 0x10) right += ret1;
	if (pan & 0x20) right += ret2;
	if (pan & 0x40) right += ret3;
	if (pan & 0x80) right += ret4;

	leftTotal = double(left * volL);
	rightTotal = double(right * volR);
	// leftTotal += double(left * volL * lowPassFilterVals[sample % std::size(lowPassFilterVals) ]);
	// rightTotal += double(right * volR * lowPassFilterVals[sample % std::size(lowPassFilterVals)]);

	sample++;
	int max = OUTPUT_SAMPLES;

	// The gameboy frequency is not divisible by the output sample rate (48000)
	// Add an extra sample when needed to keep the sample rate at 48000
	//max += remainders < OUTPUT_SAMPLES_REMAINDER  ? 1 : 0;

	sample %= max;
	if (sample == 0) {
		buf.push_back((float) leftTotal / 200.0);
		buf.push_back((float) rightTotal / 200.0);

		/*if (remainders-- < 0) {
			remainders = OUTPUT_SAMPLES_REMAINDER_MAX;
		}*/

		leftTotal = 0;
		rightTotal = 0;

		if (buf.size() == OUTPUT_BUFFER_SIZE * 2) {
			SDL_QueueAudio(dev, buf.data(), (Uint32) buf.size() * sizeof(float));
			buf.clear();
		}
	}
}