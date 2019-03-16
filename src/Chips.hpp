#pragma once

#include "CPU.hpp"
#include "Memory.hpp"

namespace atre
{

enum ChipRegisters
{
	// GTIA
	PAL = 0xD014,
	COLBK = 0xD01A,
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
	DMACTL = 0xD400
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
};

class GTIA : public Chip
{
  public:
	GTIA(CPU *cpu, Memory *memory) : Chip(cpu, memory) {}

	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
};

class ANTIC : public Chip
{
	word_t _lineNum;
	word_t _lineCycle;

  public:
	ANTIC(CPU *cpu, Memory *memory) : Chip(cpu, memory) {}

	void Reset() override;
	void Tick() override;
	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
};

class POKEY : public Chip
{
	std::vector<byte_t> _serialOut;
	byte_t _irqStatus;
	bool _sioComplete;

  public:
	POKEY(CPU *cpu, Memory *memory) : Chip(cpu, memory), _serialOut(),
									  _irqStatus(0), _sioComplete(false) {}

	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
	void Tick() override;
};

class PIA : public Chip
{
  public:
	PIA(CPU *cpu, Memory *memory) : Chip(cpu, memory) {}

	void Reset() override;
	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
};

class ChipIO
{
  protected:
	GTIA _GTIA;
	ANTIC _ANTIC;
	POKEY _POKEY;
	PIA _PIA;

  public:
	ChipIO(CPU *cpu, Memory *memory);
	void Tick();
	void Reset();
	void Write(word_t reg, byte_t val);
	byte_t Read(word_t reg);
};

} // namespace atre
