#pragma once

#include "atre.hpp"
#include "Memory.hpp"

namespace atre
{
class Debugger;
class Tests;

enum class Addressing
{
	Immediate = 0,
	Absolute = 1,
	ZeroPage = 2,
	ZeroPageX = 3,
	AbsoluteX = 4,
	AbsoluteY = 5,
	IndexedIndirect = 6,
	IndirectIndexed = 7
};

class CPU
{
	friend class Debugger;
	friend class Tests;

  public:
	CPU(Memory &memory);
	void Reset();
	void Execute();

  protected:
	byte_t A;
	byte_t X;
	byte_t Y;
	byte_t S;
	word_t PC;
	byte_t F;
	unsigned long _cycles;

	Memory &mMemory;

	const static flag_t NEGATIVE_FLAG = 0b10000000;
	const static flag_t OVERFLOW_FLAG = 0b01000000;
	const static flag_t IGNORED_FLAG = 0b00100000;
	const static flag_t BREAK_FLAG = 0b00010000;
	const static flag_t DECIMAL_FLAG = 0b00001000;
	const static flag_t INTERRUPT_FLAG = 0b00000100;
	const static flag_t ZERO_FLAG = 0b00000010;
	const static flag_t CARRY_FLAG = 0b00000001;

	typedef void (atre::CPU::*OPFunction)(word_t, Addressing);
	typedef std::tuple<OPFunction, Addressing, int /*bytes*/, int /*cycles*/> OPCode;

	OPCode _opCodeMap[256];

	void InitializeOPCodes();
	void SetFlag(flag_t flag);
	void SetFlag(flag_t flag, bool isSet);
	void ClearFlag(flag_t flag);
	bool IsSetFlag(flag_t flag) const;

	void Cycles(unsigned long cycles);
	static bool IsNegative(byte_t op);
	static bool IsZero(byte_t op);

	byte_t GetOP(word_t opIndex, Addressing adr);

	void ADC(byte_t op);
	void opADC(word_t opIndex, Addressing adr);
};
} // namespace atre
