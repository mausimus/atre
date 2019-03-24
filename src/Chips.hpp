#pragma once

#include "CPU.hpp"
#include "Memory.hpp"

namespace atre
{

class ANTIC;

enum ChipRegisters
{
	// GTIA
	PAL = 0xD014,
	COLBK = 0xD01A,
	COLPF0 = 0xD016,
	COLPF1 = 0xD017,
	COLPF2 = 0xD018,
	COLPF3 = 0xD019,
	PRIOR = 0xD01B,
	VDELAY = 0xD01C,
	CONSOL = 0xD01F,
	GRAFP0 = 0xD00D,
	GRAFP1 = 0xD00E,
	GRAFP2 = 0xD00F,
	GRAFP3 = 0xD010,
	GRAFM = 0xD011,
	GRACTL = 0xD01D,
	HPOSP0 = 0xD000,
	HPOSP1 = 0xD001,
	HPOSP2 = 0xD002,
	HPOSP3 = 0xD003,
	HPOSM0 = 0xD004,
	HPOSM1 = 0xD005,
	HPOSM2 = 0xD006,
	HPOSM3 = 0xD007,
	SIZEP0 = 0xD008,
	SIZEP1 = 0xD009,
	SIZEP2 = 0xD00A,
	SIZEP3 = 0xD00B,
	SIZEM = 0xD00C,
	COLPM0 = 0xD012,
	COLPM1 = 0xD013,
	COLPM2 = 0xD014,
	COLPM3 = 0xD015,
	TRIG0 = 0xD010,
	TRIG1 = 0xD011,
	M0PF = 0xD000,
	M1PF = 0xD001,
	M2PF = 0xD002,
	M3PF = 0xD003,
	P0PF = 0xD004,
	P1PF = 0xD005,
	P2PF = 0xD006,
	P3PF = 0xD007,
	M0PL = 0xD008,
	M1PL = 0xD009,
	M2PL = 0xD00A,
	M3PL = 0xD00B,
	P0PL = 0xD00C,
	P1PL = 0xD00D,
	P2PL = 0xD00E,
	P3PL = 0xD00F,
	HITCLR = 0xD01E,

	// POKEY
	STIMER = 0xD209,
	KBCODE = 0xD209,
	IRQEN = 0xD20E,
	IRQST = 0xD20E,
	SEROUT = 0xD20D,
	SERIN = 0xD20D,
	SKSTAT = 0xD20F,
	AUDF1 = 0xD200,
	AUDF2 = 0xD202,
	AUDF3 = 0xD204,
	AUDF4 = 0xD206,
	AUDCTL = 0xD208,

	// PIA
	PORTA = 0xD300,
	PORTB = 0xD301,

	// ANTIC
	VCOUNT = 0xD40B,
	NMIEN = 0xD40E,
	NMIST = 0xD40F,
	NMIRES = 0xD40F,
	DLISTL = 0xD402,
	DMACTL = 0xD400,
	PMBASE = 0xD407,
	CHBASE = 0xD409,
	CHACTL = 0xD401,
	WSYNC = 0xD40A,
	HSCROL = 0xD404
};

class Chip
{
  protected:
	CPU *mCPU;
	Memory *mMemory;

  public:
	Chip(CPU *cpu, Memory *memory) : mCPU(cpu), mMemory(memory) {}
	virtual void Write(word_t reg, byte_t val) = 0;
	virtual byte_t Read(word_t reg) = 0;
	virtual void Tick() {}
	virtual void Reset() {}

	virtual ~Chip() {}
};

class GTIA : public Chip
{
	ANTIC *_ANTIC;

	bool _optionKey;
	bool _selectKey;
	bool _startKey;
	bool _joyFire;

	word_t _lineNum;
	word_t _lineCycle;
	uint32_t *_frameBuffer;

	byte_t _collisions[16];
	byte_t _playerPos[FRAME_WIDTH];

	bool DrawMissile(word_t posRegister, word_t colorRegister, int shift, word_t collisionRegister);
	bool DrawPlayer(word_t posRegister, word_t colorRegister, word_t maskRegister, word_t sizeRegister, word_t collisionRegister, int num);

  public:
	GTIA(CPU *cpu, Memory *memory, ANTIC *antic) : Chip(cpu, memory), _ANTIC(antic), _optionKey(), _selectKey(), _startKey(), _joyFire()
	{
	}

	void Tick() override;
	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;

	void Option(bool state) { _optionKey = state; };
	void Select(bool state) { _selectKey = state; };
	void Start(bool state) { _startKey = state; };

	void JoyFire(bool state) { _joyFire = state; }
};

class POKEY : public Chip
{
	std::vector<byte_t> _serialOut;

	byte_t _irqStatus;
	byte_t _kbCode;
	byte_t _kbStat;
	bool _sioComplete;
	bool _keyPressed;
	bool _breakPressed;
	unsigned long _cycles;

  public:
	void KeyDown(byte_t scanCode, bool shiftStatus, bool ctrlStatus);
	void KeyUp();
	void Break();
	void ShiftKey(bool shiftStatus);

	POKEY(CPU *cpu, Memory *memory) : Chip(cpu, memory),
									  _serialOut(),
									  _irqStatus(0),
									  _kbCode(),
									  _sioComplete(false),
									  _cycles(0)
	{
	}

	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
	void Tick() override;
}; // namespace atre

class PIA : public Chip
{
	bool _joyUp;
	bool _joyDown;
	bool _joyLeft;
	bool _joyRight;

  public:
	PIA(CPU *cpu, Memory *memory) : Chip(cpu, memory),
									_joyUp(), _joyDown(), _joyLeft(), _joyRight() {}

	void JoyUp(bool state) { _joyUp = state; }
	void JoyDown(bool state) { _joyDown = state; }
	void JoyLeft(bool state) { _joyLeft = state; }
	void JoyRight(bool state) { _joyRight = state; }

	void Reset() override;
	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
};

} // namespace atre
