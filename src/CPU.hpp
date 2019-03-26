#pragma once

#include "Debugger.hpp"
#include "RAM.hpp"
#include "atre.hpp"

namespace atre
{
class Tests;
class IO;

enum class AddressingMode
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
	CPU(RAM* ram);

	void Attach(Callbacks* callbacks);
	void Connect(IO* io);
	void Reset();
	void IRQ();
	void NMI();
	void JumpTo(word_t startAddr);
	void BreakAt(word_t startAddr);
	void Execute();
	void Wait(unsigned cycles);

	inline unsigned long Cycles() const
	{
		return m_cycles;
	}

	bool										m_showCycles;
	bool										m_enableTraps;
	bool										m_showSteps;
	std::vector<std::pair<word_t, std::string>> m_callStack;

protected:
	typedef void (atre::CPU::*OPFunction)(word_t, AddressingMode);
	typedef std::tuple<OPFunction, AddressingMode, int /*bytes*/, int /*cycles*/> OPCode;

	OPCode		  m_opCodeMap[256];
	byte_t		  A;
	byte_t		  X;
	byte_t		  Y;
	byte_t		  S;
	word_t		  PC;
	byte_t		  F;
	word_t		  BRK;
	unsigned long m_cycles;
	unsigned long m_seconds;
	bool		  m_irqPending;
	bool		  m_nmiPending;
	unsigned	  m_waitCycles;
	RAM*		  m_RAM;
	Callbacks*	  m_callbacks;
	IO*			  m_IO;

	const static flag_t NEGATIVE_FLAG  = 0b10000000;
	const static flag_t OVERFLOW_FLAG  = 0b01000000;
	const static flag_t IGNORED_FLAG   = 0b00100000; // bit 5
	const static flag_t BREAK_FLAG	   = 0b00010000; // bit 4
	const static flag_t DECIMAL_FLAG   = 0b00001000;
	const static flag_t INTERRUPT_FLAG = 0b00000100;
	const static flag_t ZERO_FLAG	   = 0b00000010;
	const static flag_t CARRY_FLAG	   = 0b00000001;

	static bool IsNegative(byte_t op);
	static bool IsZero(byte_t op);

	void   InitializeOPCodes();
	void   SetFlag(flag_t flag);
	void   SetFlag(flag_t flag, bool isSet);
	void   ClearFlag(flag_t flag);
	bool   IsSetFlag(flag_t flag) const;
	void   doIRQ();
	void   doNMI();
	void   Cycles(unsigned long cycles);
	void   StackPush(byte_t val);
	byte_t StackPull();
	byte_t GetOP(word_t opIndex, AddressingMode adr);
	void   SetOP(word_t opIndex, AddressingMode adr, byte_t val);

	void   ADC(byte_t op);
	void   opADC(word_t opIndex, AddressingMode adr);
	void   AND(byte_t op);
	void   opAND(word_t opIndex, AddressingMode adr);
	byte_t ASL(byte_t op);
	void   opASL(word_t opIndex, AddressingMode adr);
	void   Branch(word_t opIndex, bool condition);
	void   opBCC(word_t opIndex, AddressingMode adr);
	void   opBCS(word_t opIndex, AddressingMode adr);
	void   opBEQ(word_t opIndex, AddressingMode adr);
	void   opBMI(word_t opIndex, AddressingMode adr);
	void   opBNE(word_t opIndex, AddressingMode adr);
	void   opBPL(word_t opIndex, AddressingMode adr);
	void   opBVC(word_t opIndex, AddressingMode adr);
	void   opBVS(word_t opIndex, AddressingMode adr);
	void   opBIT(word_t opIndex, AddressingMode adr);
	void   opBRK(word_t opIndex, AddressingMode adr);
	void   opCLC(word_t opIndex, AddressingMode adr);
	void   opCLD(word_t opIndex, AddressingMode adr);
	void   opCLI(word_t opIndex, AddressingMode adr);
	void   opCLV(word_t opIndex, AddressingMode adr);
	void   Compare(byte_t r, byte_t op);
	void   opCMP(word_t opIndex, AddressingMode adr);
	void   opCPX(word_t opIndex, AddressingMode adr);
	void   opCPY(word_t opIndex, AddressingMode adr);
	void   opDEC(word_t opIndex, AddressingMode adr);
	void   opDEX(word_t opIndex, AddressingMode adr);
	void   opDEY(word_t opIndex, AddressingMode adr);
	void   opINC(word_t opIndex, AddressingMode adr);
	void   opINX(word_t opIndex, AddressingMode adr);
	void   opINY(word_t opIndex, AddressingMode adr);
	void   EOR(byte_t op);
	void   opEOR(word_t opIndex, AddressingMode adr);
	void   opJMP(word_t opIndex, AddressingMode adr);
	void   opJSR(word_t opIndex, AddressingMode adr);
	void   opRTS(word_t opIndex, AddressingMode adr);
	void   opLDA(word_t opIndex, AddressingMode adr);
	void   opLDX(word_t opIndex, AddressingMode adr);
	void   opLDY(word_t opIndex, AddressingMode adr);
	byte_t LSR(byte_t op);
	void   opLSR(word_t opIndex, AddressingMode adr);
	void   opNOP(word_t opIndex, AddressingMode adr);
	void   ORA(byte_t op);
	void   opORA(word_t opIndex, AddressingMode adr);
	void   opPHA(word_t opIndex, AddressingMode adr);
	void   opPHP(word_t opIndex, AddressingMode adr);
	void   opPLA(word_t opIndex, AddressingMode adr);
	void   opPLP(word_t opIndex, AddressingMode adr);
	byte_t ROL(byte_t op);
	void   opROL(word_t opIndex, AddressingMode adr);
	byte_t ROR(byte_t op);
	void   opROR(word_t opIndex, AddressingMode adr);
	void   opRTI(word_t opIndex, AddressingMode adr);
	void   SBC(byte_t op);
	void   opSBC(word_t opIndex, AddressingMode adr);
	void   opSEC(word_t opIndex, AddressingMode adr);
	void   opSED(word_t opIndex, AddressingMode adr);
	void   opSEI(word_t opIndex, AddressingMode adr);
	void   opSTA(word_t opIndex, AddressingMode adr);
	void   opSTX(word_t opIndex, AddressingMode adr);
	void   opSTY(word_t opIndex, AddressingMode adr);
	void   opTAX(word_t opIndex, AddressingMode adr);
	void   opTAY(word_t opIndex, AddressingMode adr);
	void   opTSX(word_t opIndex, AddressingMode adr);
	void   opTXA(word_t opIndex, AddressingMode adr);
	void   opTXS(word_t opIndex, AddressingMode adr);
	void   opTYA(word_t opIndex, AddressingMode adr);
};
} // namespace atre
