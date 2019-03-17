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

	// POKEY
	STIMER = 0xD209,
	KBCODE = 0xD209,
	IRQEN = 0xD20E,
	IRQST = 0xD20E,
	SEROUT = 0xD20D,
	SERIN = 0xD20D,
	SKSTAT = 0xD20F,

	// PIA
	PORTB = 0xD301,

	// ANTIC
	VCOUNT = 0xD40B,
	NMIEN = 0xD40E,
	NMIST = 0xD40F,
	NMIRES = 0xD40F,
	DLISTL = 0xD402,
	DMACTL = 0xD400,
	CHBASE = 0xD409,
	CHACTL = 0xD401
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
  public:
	GTIA(CPU *cpu, Memory *memory) : Chip(cpu, memory) {}

	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
};

class POKEY : public Chip
{
	std::vector<byte_t> _serialOut;
	std::string _keyBuffer; // TODO

	//	static std::map<char, byte_t> _scanCodes;

	byte_t _irqStatus;
	byte_t _kbCode;
	byte_t _kbStat;
	bool _sioComplete;
	bool _keyPressed;
	bool _breakPressed;
	unsigned long _cycles;

  public:
	void KeyboardInput(const std::string &input);
	void KeyDown(byte_t scanCode, bool shiftStatus, bool ctrlStatus);
	void KeyUp();
	void Break();
	void ShiftKey(bool shiftStatus);

	POKEY(CPU *cpu, Memory *memory) : Chip(cpu, memory), _serialOut(),
									  _keyBuffer(),
									  _irqStatus(0), _kbCode(), _sioComplete(false), _cycles(0) {}

	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
	void Tick() override;
}; // namespace atre

class PIA : public Chip
{
  public:
	PIA(CPU *cpu, Memory *memory) : Chip(cpu, memory) {}

	void Reset() override;
	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
};

} // namespace atre
