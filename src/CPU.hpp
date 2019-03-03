#pragma once

#include "atre.hpp"
#include "Memory.hpp"

namespace atre
{
class Debugger;
class Tests;

enum class Addressing
{
	None,
	Immediate,
	Absolute,
	ZeroPage,
	ZeroPageX,
	ZeroPageY,
	AbsoluteX,
	AbsoluteY,
	IndexedIndirect,
	IndirectIndexed,
	Accumulator,
	Relative,
	Indirect,
	AbsoluteXPaged,
	AbsoluteYPaged,
	IndirectIndexedPaged
};

class CPU
{
	friend class Debugger;
	friend class Tests;

  public:
	CPU(Memory *memory);
	void Reset();
	void IRQ();
	void NMI();
	void JumpTo(word_t startAddr);
	void Execute();
	void ExecuteUntil(word_t endAddr);
	void Dump();
	unsigned long Cycles() const;

	bool mShowCycles;
	bool mEnableTraps;
	bool mDumpState;

  protected:
	byte_t A;
	byte_t X;
	byte_t Y;
	byte_t S;
	word_t PC;
	byte_t F;
	word_t EC; // "Exit counter"
	unsigned long _cycles;
	unsigned long _seconds;
	bool _irqPending;
	bool _nmiPending;

	Memory *mMemory;

	const static flag_t NEGATIVE_FLAG = 0b10000000;
	const static flag_t OVERFLOW_FLAG = 0b01000000;
	const static flag_t IGNORED_FLAG = 0b00100000; // bit 5
	const static flag_t BREAK_FLAG = 0b00010000;   // bit 4
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

	void StackPush(byte_t val);
	byte_t StackPull();

	void Cycles(unsigned long cycles);
	static bool IsNegative(byte_t op);
	static bool IsZero(byte_t op);

	byte_t GetOP(word_t opIndex, Addressing adr);
	void SetOP(word_t opIndex, Addressing adr, byte_t val);

	void ADC(byte_t op);
	void opADC(word_t opIndex, Addressing adr);

	void AND(byte_t op);
	void opAND(word_t opIndex, Addressing adr);

	byte_t ASL(byte_t op);
	void opASL(word_t opIndex, Addressing adr);

	void Branch(word_t opIndex, bool condition);
	void opBCC(word_t opIndex, Addressing adr);
	void opBCS(word_t opIndex, Addressing adr);
	void opBEQ(word_t opIndex, Addressing adr);
	void opBMI(word_t opIndex, Addressing adr);
	void opBNE(word_t opIndex, Addressing adr);
	void opBPL(word_t opIndex, Addressing adr);
	void opBVC(word_t opIndex, Addressing adr);
	void opBVS(word_t opIndex, Addressing adr);

	void opBIT(word_t opIndex, Addressing adr);
	void opBRK(word_t opIndex, Addressing adr);

	void opCLC(word_t opIndex, Addressing adr);
	void opCLD(word_t opIndex, Addressing adr);
	void opCLI(word_t opIndex, Addressing adr);
	void opCLV(word_t opIndex, Addressing adr);

	void Compare(byte_t r, byte_t op);
	void opCMP(word_t opIndex, Addressing adr);
	void opCPX(word_t opIndex, Addressing adr);
	void opCPY(word_t opIndex, Addressing adr);

	void opDEC(word_t opIndex, Addressing adr);
	void opDEX(word_t opIndex, Addressing adr);
	void opDEY(word_t opIndex, Addressing adr);

	void opINC(word_t opIndex, Addressing adr);
	void opINX(word_t opIndex, Addressing adr);
	void opINY(word_t opIndex, Addressing adr);

	void EOR(byte_t op);
	void opEOR(word_t opIndex, Addressing adr);

	void opJMP(word_t opIndex, Addressing adr);

	void opJSR(word_t opIndex, Addressing adr);
	void opRTS(word_t opIndex, Addressing adr);

	void opLDA(word_t opIndex, Addressing adr);
	void opLDX(word_t opIndex, Addressing adr);
	void opLDY(word_t opIndex, Addressing adr);

	byte_t LSR(byte_t op);
	void opLSR(word_t opIndex, Addressing adr);

	void opNOP(word_t opIndex, Addressing adr);

	void ORA(byte_t op);
	void opORA(word_t opIndex, Addressing adr);

	void opPHA(word_t opIndex, Addressing adr);
	void opPHP(word_t opIndex, Addressing adr);
	void opPLA(word_t opIndex, Addressing adr);
	void opPLP(word_t opIndex, Addressing adr);

	byte_t ROL(byte_t op);
	void opROL(word_t opIndex, Addressing adr);

	byte_t ROR(byte_t op);
	void opROR(word_t opIndex, Addressing adr);

	void opRTI(word_t opIndex, Addressing adr);

	void SBC(byte_t op);
	void opSBC(word_t opIndex, Addressing adr);

	void opSEC(word_t opIndex, Addressing adr);
	void opSED(word_t opIndex, Addressing adr);
	void opSEI(word_t opIndex, Addressing adr);

	void opSTA(word_t opIndex, Addressing adr);
	void opSTX(word_t opIndex, Addressing adr);
	void opSTY(word_t opIndex, Addressing adr);

	void opTAX(word_t opIndex, Addressing adr);
	void opTAY(word_t opIndex, Addressing adr);
	void opTSX(word_t opIndex, Addressing adr);
	void opTXA(word_t opIndex, Addressing adr);
	void opTXS(word_t opIndex, Addressing adr);
	void opTYA(word_t opIndex, Addressing adr);
};
} // namespace atre
