#pragma once
#include <gb/gbspace.h>
#include <gb/gbtimer.h>
#include <SDL.h>
#include <array>
#include <BaseTsd.h>

#define SQUARE_FREQUENCY_PERIOD(freq) ((2048 - freq))
#define WAVE_FREQUENCY_PERIOD(freq) ((2048 - freq))
#define NOISE_FREQUENCY_PERIOD(divisor, shift) ((divisor << shift))

#define MAX_VOLUME 15.0

#define OUTPUT_FREQ 32768
//#define OUTPUT_FREQ 48000
#define OUTPUT_SAMPLES GB_HZ / 4 / OUTPUT_FREQ
#define OUTPUT_SAMPLES_REMAINDER 316
#define OUTPUT_SAMPLES_REMAINDER_MAX 375
#define OUTPUT_BUFFER_SIZE 512

namespace gb::sound {

class gbSC1 : public gbSpace {
	int volume = 0;
	int timer = 0;
	int envelopeTimer = 0;
	int sweepTimer = 0;
	bool sweepEnabled = false;
	int frequencyShadow = 0;

	void shiftFreqCheck(bool);

	void triggerSound();
public:
	bool enabled = false;
	int cycle = 0;

	union {
		UINT8 mem[5]{ 0x80, 0x80, 0xF3, 0x00, 0x00 };
		struct {
			// NR10
			UINT8 nr10SwpStep : 3;
			UINT8 nr10SwpDir : 1;
			UINT8 nr10SwpPace : 3;
			UINT8 nr10res : 1;

			// NR11
			UINT8 nr11Len : 6;
			UINT8 nr11DutyCycle : 2;

			// NR12
			UINT8 nr12EnvPeriod : 3;
			UINT8 nr12EnvDir : 1;
			UINT8 nr12Vol : 4;

			// NR13-NR14
			UINT16 nr13Freq : 11;
			UINT16 nr14res : 3;
			UINT16 nr14LenEnable : 1;
			UINT16 nr14Trigger : 1;
		};
	};

	double step(bool lengthClock, bool volumeClock, bool sweepClock);
	virtual int writeByte(UINT16 addr, UINT8 byte) override;
	virtual int readByte(UINT16 addr) override;
};

class gbSC2 : public gbSpace {
	int volume = 0;
	int timer = 0;
	int envelopeTimer = 0;

	void triggerSound();
public:
	bool enabled = false;
	int cycle = 0;

	union {
		UINT8 mem[5]{ 0 };
		struct {
			UINT8 nr20res;

			// NR21
			UINT8 length : 6;
			UINT8 dutyCycle : 2;

			// NR22
			UINT8 envelopePeriod : 3;
			UINT8 envelopeAdd : 1;
			UINT8 startingVolume : 4;

			// NR23-NR24
			UINT16 frequency : 11;
			UINT16 nr14res : 3;
			UINT16 counter : 1;
			UINT16 trigger : 1;
		};
	};

	double step(bool lengthClock, bool volumeClock, bool sweepClock);
	virtual int writeByte(UINT16 addr, UINT8 byte) override;
	virtual int readByte(UINT16 addr) override;
};

class gbSC3 : public gbSpace {
	UINT8 volumeCodes[4]{
		4,
		0,
		1,
		2,
	};
	
	int timer = 0;

	void triggerSound();
public:
	bool enabled = false;
	int sample = 0;

	union {
		UINT8 mem[5]{ 0 };
		struct {
			// NR30
			UINT8 nr30res : 7;
			UINT8 dacPower : 1;

			// NR31
			UINT8 length;

			// NR32
			UINT8 nr32res1 : 5;
			UINT8 volume : 2;
			UINT8 nr32res2 : 1;

			// NR23-NR24
			UINT16 frequency : 11;
			UINT16 nr34res : 3;
			UINT16 counter : 1;
			UINT16 trigger : 1;
		};
	};

	UINT8 waveTable[16]{0};

	double step(bool lengthClock, bool volumeClock, bool sweepClock);
	virtual int writeByte(UINT16 addr, UINT8 byte) override;
	virtual int readByte(UINT16 addr) override;
};

class gbSC4 : public gbSpace {
	int volume = 0;
	int timer = 0;
	int envelopeTimer = 0;

	UINT8 divisorCodes[8]{ 8, 16, 32, 48, 64, 80, 96, 112 };

	void triggerSound();
public:
	bool enabled = false;
	UINT16 shiftRegister = 0xFFFF;

	union {
		UINT8 mem[5]{ 0 };
		struct {
			// NR40
			UINT8 nr40res;

			// NR41
			UINT8 length : 6;
			UINT8 nr41res : 2;

			// NR42
			UINT8 envelopePeriod : 3;
			UINT8 envelopeAdd : 1;
			UINT8 startingVolume : 4;

			// NR43
			UINT8 divisor : 3;
			UINT8 width : 1;
			UINT8 clockShift : 4;

			// NR44
			UINT16 nr44res : 6;
			UINT16 counter : 1;
			UINT16 trigger : 1;
		};
	};

	double step(bool lengthClock, bool volumeClock, bool sweepClock);
	virtual int writeByte(UINT16 addr, UINT8 byte) override;
	virtual int readByte(UINT16 addr) override;
};

constexpr size_t SOUND_BUF_SIZE = 512;
class APU : public gbSpace {
public:
	union {
		UINT8 mem[3]{ 0x77, 0xF3, 0x80 };
		struct {
			// NR50
			UINT8 volR : 3;
			UINT8 vinR : 1;
			UINT8 volL : 3;
			UINT8 vinL : 1;

			// NR51
			UINT8 pan;

			// NR52
			UINT8 res : 7;
			UINT8 enable : 1;
		};
	};

	APU(gb::timer::Timer &timer);

	virtual int writeByte(UINT16 addr, UINT8 byte) override;
	virtual int readByte(UINT16 addr) override;

	void step();
	SDL_AudioDeviceID dev;

private:
	double sampleLeft = 0;
	double sampleRight = 0;
	int sample = 0;
	size_t bufIdx { 0 };

	bool poweredOn = true;

	gbSC1 sc1;
	gbSC2 sc2;
	gbSC3 sc3;
	gbSC4 sc4;

	timer::Timer &mTimer;
	utils::Clock mDivApuClk;
	utils::FallingEdgeDetector mDivApuDet;
	utils::FallingEdgeDetector mLengthDet;
	utils::FallingEdgeDetector mSweepDet;
	utils::FallingEdgeDetector mEnvelopeDet;

	std::array<float, SOUND_BUF_SIZE * 2> buf;
};

}; // namespace gb::sound