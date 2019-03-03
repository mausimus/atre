#include "CPU.hpp"

using namespace std;

namespace atre
{
CPU::CPU(Memory *memory) : mShowCycles(false), mEnableTraps(true), mDumpState(false), _cycles(0), _seconds(0), mMemory(memory)
{
	InitializeOPCodes();
	Reset();
}

void CPU::Reset()
{
	A = X = Y = 0;
	F = CPU::IGNORED_FLAG;
	S = 0xFF;
	PC = 0xFFFC;
	_cycles = 0;
	_seconds = 0;
	_irqPending = false;
	_nmiPending = false;
}

void CPU::SetFlag(flag_t flag)
{
	F |= flag;
}

void CPU::ClearFlag(flag_t flag)
{
	F &= ~flag;
}

bool CPU::IsSetFlag(flag_t flag) const
{
	return F & flag;
}

void CPU::SetFlag(flag_t flag, bool isSet)
{
	if (isSet)
	{
		SetFlag(flag);
	}
	else
	{
		ClearFlag(flag);
	}
}

void CPU::StackPush(byte_t val)
{
	mMemory->Set(0x100 + S, val);
	S--;
}

byte_t CPU::StackPull()
{
	S++;
	return mMemory->Get(0x100 + S);
}

void CPU::Cycles(unsigned long cycles)
{
	_cycles += cycles;
	if (_cycles >= CYCLES_PER_SEC)
	{
		_seconds++;
		_cycles -= CYCLES_PER_SEC;
		if (mShowCycles)
		{
			cout << "#" << flush;
		}
	}
}

byte_t CPU::GetOP(word_t opIndex, Addressing adr)
{
	switch (adr)
	{
	case Addressing::Accumulator:
	{
		return A;
	}
	case Addressing::Immediate:
	{
		return mMemory->Get(opIndex);
	}
	case Addressing::Absolute:
	{
		word_t addr = mMemory->GetW(opIndex);
		return mMemory->Get(addr);
	}
	case Addressing::ZeroPage:
	{
		return mMemory->Get(mMemory->Get(opIndex));
	}
	case Addressing::ZeroPageX:
	{
		return mMemory->Get((mMemory->Get(opIndex) + X) & 0xFF);
	}
	case Addressing::ZeroPageY:
	{
		return mMemory->Get((mMemory->Get(opIndex) + Y) & 0xFF);
	}
	case Addressing::AbsoluteX:
	case Addressing::AbsoluteXPaged:
	{
		word_t baseAddr = mMemory->GetW(opIndex);
		auto finalAddr = baseAddr + X;
		if (adr == Addressing::AbsoluteXPaged && baseAddr >> 8 != finalAddr >> 8)
		{
			Cycles(1);
		}
		return mMemory->Get(finalAddr);
	}
	case Addressing::AbsoluteY:
	case Addressing::AbsoluteYPaged:
	{
		word_t baseAddr = mMemory->GetW(opIndex);
		auto finalAddr = baseAddr + Y;
		if (adr == Addressing::AbsoluteYPaged && baseAddr >> 8 != finalAddr >> 8)
		{
			Cycles(1);
		}
		return mMemory->Get(finalAddr);
	}
	case Addressing::IndexedIndirect:
	{
		word_t baseAddr = (mMemory->Get(opIndex) + X) & 0xFF;
		word_t loTarget = mMemory->Get(baseAddr);
		word_t hiTarget = mMemory->Get((baseAddr + 1) & 0xFF);
		word_t addr = loTarget + (hiTarget << 8);
		return mMemory->Get(addr);
	}
	case Addressing::IndirectIndexed:
	case Addressing::IndirectIndexedPaged:
	{
		byte_t basePointer = mMemory->Get(opIndex);
		word_t loTarget = mMemory->Get(basePointer);
		word_t hiTarget = mMemory->Get((basePointer + 1) & 0xFF);
		word_t baseAddress = loTarget + (hiTarget << 8);
		word_t finalAddress = baseAddress + Y;
		if (adr == Addressing::IndirectIndexedPaged && baseAddress >> 8 != finalAddress >> 8)
		{
			Cycles(1);
		}
		return mMemory->Get(finalAddress);
	}
	default:
		throw std::runtime_error("Not supported addressing mode");
	}
}

void CPU::SetOP(word_t opIndex, Addressing adr, byte_t val)
{
	switch (adr)
	{
	case Addressing::Accumulator:
	{
		A = val;
		break;
	}
	case Addressing::Immediate:
	{
		mMemory->Set(opIndex, val);
		break;
	}
	case Addressing::Absolute:
	{
		word_t addr = mMemory->GetW(opIndex);
		mMemory->Set(addr, val);
		break;
	}
	case Addressing::ZeroPage:
	{
		mMemory->Set(mMemory->Get(opIndex), val);
		break;
	}
	case Addressing::ZeroPageX:
	{
		mMemory->Set((mMemory->Get(opIndex) + X) & 0xFF, val);
		break;
	}
	case Addressing::ZeroPageY:
	{
		mMemory->Set((mMemory->Get(opIndex) + Y) & 0xFF, val);
		break;
	}
	case Addressing::AbsoluteX:
	{
		word_t baseAddr = mMemory->GetW(opIndex);
		auto finalAddr = baseAddr + X;
		mMemory->Set(finalAddr, val);
		break;
	}
	case Addressing::AbsoluteY:
	{
		word_t baseAddr = mMemory->GetW(opIndex);
		auto finalAddr = baseAddr + Y;
		mMemory->Set(finalAddr, val);
		break;
	}
	case Addressing::IndexedIndirect:
	{
		word_t baseAddr = (mMemory->Get(opIndex) + X) & 0xFF;
		word_t loTarget = mMemory->Get(baseAddr);
		word_t hiTarget = mMemory->Get((baseAddr + 1) & 0xFF);
		word_t addr = loTarget + (hiTarget << 8);
		mMemory->Set(addr, val);
		break;
	}
	case Addressing::IndirectIndexed:
	{
		byte_t basePointer = mMemory->Get(opIndex);
		word_t loTarget = mMemory->Get(basePointer);
		word_t hiTarget = mMemory->Get((basePointer + 1) & 0xFF);
		word_t baseAddress = loTarget + (hiTarget << 8);
		word_t finalAddress = baseAddress + Y;
		mMemory->Set(finalAddress, val);
		break;
	}
	default:
		throw std::runtime_error("Not supported addressing mode");
	}
}

void CPU::ADC(byte_t op)
{
	if (IsSetFlag(CPU::DECIMAL_FLAG))
	{
		auto lhr = (A & 15) + (A >> 4) * 10;
		auto rhr = (op & 15) + (op >> 4) * 10;
		auto res = lhr + rhr + (IsSetFlag(CPU::CARRY_FLAG) ? 1 : 0);
		SetFlag(CPU::CARRY_FLAG, res > 99);
		res %= 100;
		A = ((res / 10) << 4) + (res % 10);
		SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
		SetFlag(CPU::ZERO_FLAG, IsZero(res));
	}
	else
	{
		word_t wresult = A + op + (IsSetFlag(CPU::CARRY_FLAG) ? 1 : 0);
		byte_t res = (byte_t)wresult;
		SetFlag(CPU::CARRY_FLAG, wresult > 255);
		SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
		SetFlag(CPU::ZERO_FLAG, IsZero(res));
		SetFlag(CPU::OVERFLOW_FLAG, IsNegative(A ^ res) & IsNegative(op ^ res));
		A = res;
	}
}

void CPU::opADC(word_t opIndex, Addressing adr)
{
	ADC(GetOP(opIndex, adr));
}

void CPU::AND(byte_t op)
{
	A &= op;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opAND(word_t opIndex, Addressing adr)
{
	AND(GetOP(opIndex, adr));
}

byte_t CPU::ASL(byte_t op)
{
	auto res = op << 1;
	SetFlag(CPU::CARRY_FLAG, res & 0x100);
	op = static_cast<byte_t>(res & 0xFF);
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(op));
	SetFlag(CPU::ZERO_FLAG, IsZero(op));
	return op;
}

// Arithmetic Shift Left
void CPU::opASL(word_t opIndex, Addressing adr)
{
	SetOP(opIndex, adr, ASL(GetOP(opIndex, adr)));
}

void CPU::Branch(word_t opIndex, bool condition)
{
	if (condition)
	{
		Cycles(1);
		auto target = PC + static_cast<sbyte_t>(mMemory->Get(opIndex));
		if (PC >> 8 != target >> 8)
		{
			Cycles(1);
		}
		PC = target;
	}
}

// Branch on Carry Clear
void CPU::opBCC(word_t opIndex, Addressing /*adr*/)
{
	Branch(opIndex, !IsSetFlag(CPU::CARRY_FLAG));
}

// Branch on Carry Set
void CPU::opBCS(word_t opIndex, Addressing /*adr*/)
{
	Branch(opIndex, IsSetFlag(CPU::CARRY_FLAG));
}

// Branch on Result Zero
void CPU::opBEQ(word_t opIndex, Addressing /*adr*/)
{
	Branch(opIndex, IsSetFlag(CPU::ZERO_FLAG));
}

// Branch on Result Minus
void CPU::opBMI(word_t opIndex, Addressing /*adr*/)
{
	Branch(opIndex, IsSetFlag(CPU::NEGATIVE_FLAG));
}

// Branch on Result not Zero
void CPU::opBNE(word_t opIndex, Addressing /*adr*/)
{
	Branch(opIndex, !IsSetFlag(CPU::ZERO_FLAG));
}

// Branch on Result Plus
void CPU::opBPL(word_t opIndex, Addressing /*adr*/)
{
	Branch(opIndex, !IsSetFlag(CPU::NEGATIVE_FLAG));
}

// Branch on Overflow Clear
void CPU::opBVC(word_t opIndex, Addressing /*adr*/)
{
	Branch(opIndex, !IsSetFlag(CPU::OVERFLOW_FLAG));
}

// Branch on Overflow Set
void CPU::opBVS(word_t opIndex, Addressing /*adr*/)
{
	Branch(opIndex, IsSetFlag(CPU::OVERFLOW_FLAG));
}

// Test Bits in Memory with Accumulator
void CPU::opBIT(word_t opIndex, Addressing adr)
{
	byte_t op = GetOP(opIndex, adr);
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(op));
	SetFlag(CPU::OVERFLOW_FLAG, op & 0b01000000);
	byte_t res = (A & op);
	SetFlag(CPU::ZERO_FLAG, IsZero(res));
}

// external interrupt
void CPU::IRQ()
{
	StackPush(PC >> 8);
	StackPush(PC & 0xFF);
	StackPush(F);
	SetFlag(CPU::INTERRUPT_FLAG);
	PC = mMemory->GetW(0xFFFE); // jump to vector
}

// non-masked interrupt
void CPU::NMI()
{
	StackPush(PC >> 8);
	StackPush(PC & 0xFF);
	StackPush(F);
	SetFlag(CPU::INTERRUPT_FLAG);
	PC = mMemory->GetW(0xFFFA); // jump to vector
}

// Force Break
void CPU::opBRK(word_t /*opIndex*/, Addressing /*adr*/)
{
	PC++;
	StackPush(PC >> 8);
	StackPush(PC & 0xFF);
	auto f = F;
	f |= BREAK_FLAG;
	StackPush(f);
	SetFlag(CPU::INTERRUPT_FLAG);
	PC = mMemory->GetW(0xFFFE); // jump to vector
}

void CPU::opCLC(word_t /*opIndex*/, Addressing /*adr*/)
{
	ClearFlag(CPU::CARRY_FLAG);
}

void CPU::opCLD(word_t /*opIndex*/, Addressing /*adr*/)
{
	ClearFlag(CPU::DECIMAL_FLAG);
}

void CPU::opCLI(word_t /*opIndex*/, Addressing /*adr*/)
{
	ClearFlag(CPU::INTERRUPT_FLAG);
}

void CPU::opCLV(word_t /*opIndex*/, Addressing /*adr*/)
{
	ClearFlag(CPU::OVERFLOW_FLAG);
}

void CPU::Compare(byte_t r, byte_t op)
{
	auto res = static_cast<byte_t>((r - op) & 0xFF);
	SetFlag(CPU::ZERO_FLAG, IsZero(res));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
	SetFlag(CPU::CARRY_FLAG, op <= r);
}

void CPU::opCMP(word_t opIndex, Addressing adr)
{
	Compare(A, GetOP(opIndex, adr));
}

void CPU::opCPX(word_t opIndex, Addressing adr)
{
	Compare(X, GetOP(opIndex, adr));
}

void CPU::opCPY(word_t opIndex, Addressing adr)
{
	Compare(Y, GetOP(opIndex, adr));
}

void CPU::opDEC(word_t opIndex, Addressing adr)
{
	auto res = GetOP(opIndex, adr);
	res--;
	SetFlag(CPU::ZERO_FLAG, IsZero(res));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
	SetOP(opIndex, adr, res);
}

void CPU::opDEX(word_t /*opIndex*/, Addressing /*adr*/)
{
	X--;
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
}

void CPU::opDEY(word_t /*opIndex*/, Addressing /*adr*/)
{
	Y--;
	SetFlag(CPU::ZERO_FLAG, IsZero(Y));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(Y));
}

void CPU::opINC(word_t opIndex, Addressing adr)
{
	auto res = GetOP(opIndex, adr);
	res++;
	SetFlag(CPU::ZERO_FLAG, IsZero(res));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
	SetOP(opIndex, adr, res);
}

void CPU::opINX(word_t /*opIndex*/, Addressing /*adr*/)
{
	X++;
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
}

void CPU::opINY(word_t /*opIndex*/, Addressing /*adr*/)
{
	Y++;
	SetFlag(CPU::ZERO_FLAG, IsZero(Y));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(Y));
}

void CPU::EOR(byte_t op)
{
	A ^= op;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opEOR(word_t opIndex, Addressing adr)
{
	EOR(GetOP(opIndex, adr));
}

void CPU::opJMP(word_t opIndex, Addressing adr)
{
	switch (adr)
	{
	case Addressing::Absolute:
	{
		PC = mMemory->GetW(opIndex);
		break;
	}
	case Addressing::Indirect:
	{
		auto addr = mMemory->GetW(opIndex);
		PC = mMemory->GetW(addr);
		break;
	}
	default:
		throw std::runtime_error("Not supported addressing mode");
	}
}

void CPU::opJSR(word_t opIndex, Addressing /*adr*/)
{
	auto retAddress = PC - 1; // next instruction - 1
	StackPush(retAddress >> 8);
	StackPush(retAddress & 0xFF);
	PC = mMemory->GetW(opIndex);
}

void CPU::opRTS(word_t /*opIndex*/, Addressing /*adr*/)
{
	word_t retAddress = StackPull();
	retAddress += (StackPull() << 8);
	retAddress++; // we pushed next instruction - 1
	PC = retAddress;
}

void CPU::opLDA(word_t opIndex, Addressing adr)
{
	A = GetOP(opIndex, adr);
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opLDX(word_t opIndex, Addressing adr)
{
	X = GetOP(opIndex, adr);
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
}

void CPU::opLDY(word_t opIndex, Addressing adr)
{
	Y = GetOP(opIndex, adr);
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(Y));
	SetFlag(CPU::ZERO_FLAG, IsZero(Y));
}

byte_t CPU::LSR(byte_t op)
{
	SetFlag(CPU::CARRY_FLAG, op & 0x01);
	op >>= 1;
	ClearFlag(CPU::NEGATIVE_FLAG);
	SetFlag(CPU::ZERO_FLAG, IsZero(op));
	return op;
}

void CPU::opLSR(word_t opIndex, Addressing adr)
{
	SetOP(opIndex, adr, LSR(GetOP(opIndex, adr)));
}

void CPU::opNOP(word_t /*opIndex*/, Addressing /*adr*/)
{
}

void CPU::ORA(byte_t op)
{
	A |= op;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opORA(word_t opIndex, Addressing adr)
{
	ORA(GetOP(opIndex, adr));
}

void CPU::opPHA(word_t /*opIndex*/, Addressing /*adr*/)
{
	StackPush(A);
}

void CPU::opPHP(word_t /*opIndex*/, Addressing /*adr*/)
{
	auto f = F;
	f |= CPU::BREAK_FLAG;
	StackPush(f);
}

void CPU::opPLA(word_t /*opIndex*/, Addressing /*adr*/)
{
	A = StackPull();
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opPLP(word_t /*opIndex*/, Addressing /*adr*/)
{
	F = StackPull();
	ClearFlag(CPU::BREAK_FLAG);
	SetFlag(CPU::IGNORED_FLAG);
}

byte_t CPU::ROL(byte_t op)
{
	auto hadCarry = IsSetFlag(CPU::CARRY_FLAG);
	SetFlag(CPU::CARRY_FLAG, op & 0x80);
	op <<= 1;
	if (hadCarry)
	{
		op++;
	}
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(op));
	SetFlag(CPU::ZERO_FLAG, IsZero(op));
	return op;
}

void CPU::opROL(word_t opIndex, Addressing adr)
{
	SetOP(opIndex, adr, ROL(GetOP(opIndex, adr)));
}

byte_t CPU::ROR(byte_t op)
{
	auto hadCarry = IsSetFlag(CPU::CARRY_FLAG);
	SetFlag(CPU::CARRY_FLAG, op & 0x01);
	op >>= 1;
	if (hadCarry)
	{
		op += 0x80;
	}
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(op));
	SetFlag(CPU::ZERO_FLAG, IsZero(op));
	return op;
}

void CPU::opROR(word_t opIndex, Addressing adr)
{
	SetOP(opIndex, adr, ROR(GetOP(opIndex, adr)));
}

void CPU::opRTI(word_t /*opIndex*/, Addressing /*adr*/)
{
	F = StackPull();
	//ClearFlag(CPU::INTERRUPT_FLAG);
	word_t retAddress = StackPull();
	retAddress += (StackPull() << 8);
	PC = retAddress;
}

void CPU::SBC(byte_t op)
{
	if (IsSetFlag(CPU::DECIMAL_FLAG))
	{
		auto lhr = (A & 15) + (A >> 4) * 10;
		auto rhr = (op & 15) + (op >> 4) * 10;
		auto res = lhr - rhr - (IsSetFlag(CPU::CARRY_FLAG) ? 0 : 1);
		SetFlag(CPU::CARRY_FLAG, res >= 0);
		if (res < 0)
		{
			res += 100;
		}
		A = ((res / 10) << 4) + (res % 10);
		SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
		SetFlag(CPU::ZERO_FLAG, IsZero(res));
	}
	else
	{
		ADC(op ^ 0xFF);
	}
}

void CPU::opSBC(word_t opIndex, Addressing adr)
{
	SBC(GetOP(opIndex, adr));
}

void CPU::opSEC(word_t /*opIndex*/, Addressing /*adr*/)
{
	SetFlag(CPU::CARRY_FLAG);
}

void CPU::opSED(word_t /*opIndex*/, Addressing /*adr*/)
{
	SetFlag(CPU::DECIMAL_FLAG);
}

void CPU::opSEI(word_t /*opIndex*/, Addressing /*adr*/)
{
	SetFlag(CPU::INTERRUPT_FLAG);
}

void CPU::opSTA(word_t opIndex, Addressing adr)
{
	SetOP(opIndex, adr, A);
}

void CPU::opSTX(word_t opIndex, Addressing adr)
{
	SetOP(opIndex, adr, X);
}

void CPU::opSTY(word_t opIndex, Addressing adr)
{
	SetOP(opIndex, adr, Y);
}

void CPU::opTAX(word_t /*opIndex*/, Addressing /*adr*/)
{
	X = A;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
}

void CPU::opTAY(word_t /*opIndex*/, Addressing /*adr*/)
{
	Y = A;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(Y));
	SetFlag(CPU::ZERO_FLAG, IsZero(Y));
}

void CPU::opTSX(word_t /*opIndex*/, Addressing /*adr*/)
{
	X = S;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
}

void CPU::opTXA(word_t /*opIndex*/, Addressing /*adr*/)
{
	A = X;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opTXS(word_t /*opIndex*/, Addressing /*adr*/)
{
	S = X;
}

void CPU::opTYA(word_t /*opIndex*/, Addressing /*adr*/)
{
	A = Y;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

bool CPU::IsNegative(byte_t op)
{
	return op & 0b10000000;
}

bool CPU::IsZero(byte_t op)
{
	return op == 0;
}

void CPU::InitializeOPCodes()
{
	// ADC
	_opCodeMap[0x69] = std::make_tuple(&atre::CPU::opADC, Addressing::Immediate, 2, 2);
	_opCodeMap[0x65] = std::make_tuple(&atre::CPU::opADC, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0x75] = std::make_tuple(&atre::CPU::opADC, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0x6D] = std::make_tuple(&atre::CPU::opADC, Addressing::Absolute, 3, 4);
	_opCodeMap[0x7D] = std::make_tuple(&atre::CPU::opADC, Addressing::AbsoluteXPaged, 3, 4);
	_opCodeMap[0x79] = std::make_tuple(&atre::CPU::opADC, Addressing::AbsoluteYPaged, 3, 4);
	_opCodeMap[0x61] = std::make_tuple(&atre::CPU::opADC, Addressing::IndexedIndirect, 2, 6);
	_opCodeMap[0x71] = std::make_tuple(&atre::CPU::opADC, Addressing::IndirectIndexedPaged, 2, 5);

	// AND
	_opCodeMap[0x29] = std::make_tuple(&atre::CPU::opAND, Addressing::Immediate, 2, 2);
	_opCodeMap[0x25] = std::make_tuple(&atre::CPU::opAND, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0x35] = std::make_tuple(&atre::CPU::opAND, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0x2D] = std::make_tuple(&atre::CPU::opAND, Addressing::Absolute, 3, 4);
	_opCodeMap[0x3D] = std::make_tuple(&atre::CPU::opAND, Addressing::AbsoluteXPaged, 3, 4);
	_opCodeMap[0x39] = std::make_tuple(&atre::CPU::opAND, Addressing::AbsoluteYPaged, 3, 4);
	_opCodeMap[0x21] = std::make_tuple(&atre::CPU::opAND, Addressing::IndexedIndirect, 2, 6);
	_opCodeMap[0x31] = std::make_tuple(&atre::CPU::opAND, Addressing::IndirectIndexedPaged, 2, 5);

	// ASL
	_opCodeMap[0x0A] = std::make_tuple(&atre::CPU::opASL, Addressing::Accumulator, 1, 2);
	_opCodeMap[0x06] = std::make_tuple(&atre::CPU::opASL, Addressing::ZeroPage, 2, 5);
	_opCodeMap[0x16] = std::make_tuple(&atre::CPU::opASL, Addressing::ZeroPageX, 2, 6);
	_opCodeMap[0x0E] = std::make_tuple(&atre::CPU::opASL, Addressing::Absolute, 3, 6);
	_opCodeMap[0x1E] = std::make_tuple(&atre::CPU::opASL, Addressing::AbsoluteX, 3, 7);

	// branching
	_opCodeMap[0x90] = std::make_tuple(&atre::CPU::opBCC, Addressing::Relative, 2, 2);
	_opCodeMap[0xB0] = std::make_tuple(&atre::CPU::opBCS, Addressing::Relative, 2, 2);
	_opCodeMap[0xF0] = std::make_tuple(&atre::CPU::opBEQ, Addressing::Relative, 2, 2);
	_opCodeMap[0x30] = std::make_tuple(&atre::CPU::opBMI, Addressing::Relative, 2, 2);
	_opCodeMap[0xD0] = std::make_tuple(&atre::CPU::opBNE, Addressing::Relative, 2, 2);
	_opCodeMap[0x10] = std::make_tuple(&atre::CPU::opBPL, Addressing::Relative, 2, 2);
	_opCodeMap[0x50] = std::make_tuple(&atre::CPU::opBVC, Addressing::Relative, 2, 2);
	_opCodeMap[0x70] = std::make_tuple(&atre::CPU::opBVS, Addressing::Relative, 2, 2);

	// BIT
	_opCodeMap[0x24] = std::make_tuple(&atre::CPU::opBIT, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0x2C] = std::make_tuple(&atre::CPU::opBIT, Addressing::Absolute, 3, 4);

	// BRK
	_opCodeMap[0x00] = std::make_tuple(&atre::CPU::opBRK, Addressing::None, 1, 7);

	// clear flags
	_opCodeMap[0x18] = std::make_tuple(&atre::CPU::opCLC, Addressing::None, 1, 2);
	_opCodeMap[0xD8] = std::make_tuple(&atre::CPU::opCLD, Addressing::None, 1, 2);
	_opCodeMap[0x58] = std::make_tuple(&atre::CPU::opCLI, Addressing::None, 1, 2);
	_opCodeMap[0xB8] = std::make_tuple(&atre::CPU::opCLV, Addressing::None, 1, 2);

	// comparisons
	_opCodeMap[0xC9] = std::make_tuple(&atre::CPU::opCMP, Addressing::Immediate, 2, 2);
	_opCodeMap[0xC5] = std::make_tuple(&atre::CPU::opCMP, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0xD5] = std::make_tuple(&atre::CPU::opCMP, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0xCD] = std::make_tuple(&atre::CPU::opCMP, Addressing::Absolute, 3, 4);
	_opCodeMap[0xDD] = std::make_tuple(&atre::CPU::opCMP, Addressing::AbsoluteXPaged, 3, 4);
	_opCodeMap[0xD9] = std::make_tuple(&atre::CPU::opCMP, Addressing::AbsoluteYPaged, 3, 4);
	_opCodeMap[0xC1] = std::make_tuple(&atre::CPU::opCMP, Addressing::IndexedIndirect, 2, 6);
	_opCodeMap[0xD1] = std::make_tuple(&atre::CPU::opCMP, Addressing::IndirectIndexedPaged, 2, 5);
	_opCodeMap[0xE0] = std::make_tuple(&atre::CPU::opCPX, Addressing::Immediate, 2, 2);
	_opCodeMap[0xE4] = std::make_tuple(&atre::CPU::opCPX, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0xEC] = std::make_tuple(&atre::CPU::opCPX, Addressing::Absolute, 3, 4);
	_opCodeMap[0xC0] = std::make_tuple(&atre::CPU::opCPY, Addressing::Immediate, 2, 2);
	_opCodeMap[0xC4] = std::make_tuple(&atre::CPU::opCPY, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0xCC] = std::make_tuple(&atre::CPU::opCPY, Addressing::Absolute, 3, 4);

	// increment/decrement
	_opCodeMap[0xE6] = std::make_tuple(&atre::CPU::opINC, Addressing::ZeroPage, 2, 5);
	_opCodeMap[0xF6] = std::make_tuple(&atre::CPU::opINC, Addressing::ZeroPageX, 2, 6);
	_opCodeMap[0xEE] = std::make_tuple(&atre::CPU::opINC, Addressing::Absolute, 3, 6);
	_opCodeMap[0xFE] = std::make_tuple(&atre::CPU::opINC, Addressing::AbsoluteX, 3, 7);
	_opCodeMap[0xE8] = std::make_tuple(&atre::CPU::opINX, Addressing::None, 1, 2);
	_opCodeMap[0xC8] = std::make_tuple(&atre::CPU::opINY, Addressing::None, 1, 2);
	_opCodeMap[0xC6] = std::make_tuple(&atre::CPU::opDEC, Addressing::ZeroPage, 2, 5);
	_opCodeMap[0xD6] = std::make_tuple(&atre::CPU::opDEC, Addressing::ZeroPageX, 2, 6);
	_opCodeMap[0xCE] = std::make_tuple(&atre::CPU::opDEC, Addressing::Absolute, 3, 6);
	_opCodeMap[0xDE] = std::make_tuple(&atre::CPU::opDEC, Addressing::AbsoluteX, 3, 7);
	_opCodeMap[0xCA] = std::make_tuple(&atre::CPU::opDEX, Addressing::None, 1, 2);
	_opCodeMap[0x88] = std::make_tuple(&atre::CPU::opDEY, Addressing::None, 1, 2);

	// EOR
	_opCodeMap[0x49] = std::make_tuple(&atre::CPU::opEOR, Addressing::Immediate, 2, 2);
	_opCodeMap[0x45] = std::make_tuple(&atre::CPU::opEOR, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0x55] = std::make_tuple(&atre::CPU::opEOR, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0x4D] = std::make_tuple(&atre::CPU::opEOR, Addressing::Absolute, 3, 4);
	_opCodeMap[0x5D] = std::make_tuple(&atre::CPU::opEOR, Addressing::AbsoluteXPaged, 3, 4);
	_opCodeMap[0x59] = std::make_tuple(&atre::CPU::opEOR, Addressing::AbsoluteYPaged, 3, 4);
	_opCodeMap[0x41] = std::make_tuple(&atre::CPU::opEOR, Addressing::IndexedIndirect, 2, 6);
	_opCodeMap[0x51] = std::make_tuple(&atre::CPU::opEOR, Addressing::IndirectIndexedPaged, 2, 5);

	// JMP
	_opCodeMap[0x4C] = std::make_tuple(&atre::CPU::opJMP, Addressing::Absolute, 3, 3);
	_opCodeMap[0x6C] = std::make_tuple(&atre::CPU::opJMP, Addressing::Indirect, 3, 5);

	// subroutine
	_opCodeMap[0x20] = std::make_tuple(&atre::CPU::opJSR, Addressing::Absolute, 3, 6);
	_opCodeMap[0x60] = std::make_tuple(&atre::CPU::opRTS, Addressing::Absolute, 1, 6);

	// load
	_opCodeMap[0xA9] = std::make_tuple(&atre::CPU::opLDA, Addressing::Immediate, 2, 2);
	_opCodeMap[0xA5] = std::make_tuple(&atre::CPU::opLDA, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0xB5] = std::make_tuple(&atre::CPU::opLDA, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0xAD] = std::make_tuple(&atre::CPU::opLDA, Addressing::Absolute, 3, 4);
	_opCodeMap[0xBD] = std::make_tuple(&atre::CPU::opLDA, Addressing::AbsoluteXPaged, 3, 4);
	_opCodeMap[0xB9] = std::make_tuple(&atre::CPU::opLDA, Addressing::AbsoluteYPaged, 3, 4);
	_opCodeMap[0xA1] = std::make_tuple(&atre::CPU::opLDA, Addressing::IndexedIndirect, 2, 6);
	_opCodeMap[0xB1] = std::make_tuple(&atre::CPU::opLDA, Addressing::IndirectIndexedPaged, 2, 5);
	_opCodeMap[0xA2] = std::make_tuple(&atre::CPU::opLDX, Addressing::Immediate, 2, 2);
	_opCodeMap[0xA6] = std::make_tuple(&atre::CPU::opLDX, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0xB6] = std::make_tuple(&atre::CPU::opLDX, Addressing::ZeroPageY, 2, 4);
	_opCodeMap[0xAE] = std::make_tuple(&atre::CPU::opLDX, Addressing::Absolute, 3, 4);
	_opCodeMap[0xBE] = std::make_tuple(&atre::CPU::opLDX, Addressing::AbsoluteYPaged, 3, 4);
	_opCodeMap[0xA0] = std::make_tuple(&atre::CPU::opLDY, Addressing::Immediate, 2, 2);
	_opCodeMap[0xA4] = std::make_tuple(&atre::CPU::opLDY, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0xB4] = std::make_tuple(&atre::CPU::opLDY, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0xAC] = std::make_tuple(&atre::CPU::opLDY, Addressing::Absolute, 3, 4);
	_opCodeMap[0xBC] = std::make_tuple(&atre::CPU::opLDY, Addressing::AbsoluteXPaged, 3, 4);

	// LSR
	_opCodeMap[0x4A] = std::make_tuple(&atre::CPU::opLSR, Addressing::Accumulator, 1, 2);
	_opCodeMap[0x46] = std::make_tuple(&atre::CPU::opLSR, Addressing::ZeroPage, 2, 5);
	_opCodeMap[0x56] = std::make_tuple(&atre::CPU::opLSR, Addressing::ZeroPageX, 2, 6);
	_opCodeMap[0x4E] = std::make_tuple(&atre::CPU::opLSR, Addressing::Absolute, 3, 6);
	_opCodeMap[0x5E] = std::make_tuple(&atre::CPU::opLSR, Addressing::AbsoluteX, 3, 7);

	// NOP
	_opCodeMap[0xEA] = std::make_tuple(&atre::CPU::opNOP, Addressing::None, 1, 2);

	// ORA
	_opCodeMap[0x09] = std::make_tuple(&atre::CPU::opORA, Addressing::Immediate, 2, 2);
	_opCodeMap[0x05] = std::make_tuple(&atre::CPU::opORA, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0x15] = std::make_tuple(&atre::CPU::opORA, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0x0D] = std::make_tuple(&atre::CPU::opORA, Addressing::Absolute, 3, 4);
	_opCodeMap[0x1D] = std::make_tuple(&atre::CPU::opORA, Addressing::AbsoluteXPaged, 3, 4);
	_opCodeMap[0x19] = std::make_tuple(&atre::CPU::opORA, Addressing::AbsoluteYPaged, 3, 4);
	_opCodeMap[0x01] = std::make_tuple(&atre::CPU::opORA, Addressing::IndexedIndirect, 2, 6);
	_opCodeMap[0x11] = std::make_tuple(&atre::CPU::opORA, Addressing::IndirectIndexedPaged, 2, 5);

	// push/pull
	_opCodeMap[0x48] = std::make_tuple(&atre::CPU::opPHA, Addressing::IndirectIndexed, 1, 3);
	_opCodeMap[0x08] = std::make_tuple(&atre::CPU::opPHP, Addressing::IndirectIndexed, 1, 3);
	_opCodeMap[0x68] = std::make_tuple(&atre::CPU::opPLA, Addressing::IndirectIndexed, 1, 4);
	_opCodeMap[0x28] = std::make_tuple(&atre::CPU::opPLP, Addressing::IndirectIndexed, 1, 4);

	// ROL
	_opCodeMap[0x2A] = std::make_tuple(&atre::CPU::opROL, Addressing::Accumulator, 1, 2);
	_opCodeMap[0x26] = std::make_tuple(&atre::CPU::opROL, Addressing::ZeroPage, 2, 5);
	_opCodeMap[0x36] = std::make_tuple(&atre::CPU::opROL, Addressing::ZeroPageX, 2, 6);
	_opCodeMap[0x2E] = std::make_tuple(&atre::CPU::opROL, Addressing::Absolute, 3, 6);
	_opCodeMap[0x3E] = std::make_tuple(&atre::CPU::opROL, Addressing::AbsoluteX, 3, 7);

	// ROR
	_opCodeMap[0x6A] = std::make_tuple(&atre::CPU::opROR, Addressing::Accumulator, 1, 2);
	_opCodeMap[0x66] = std::make_tuple(&atre::CPU::opROR, Addressing::ZeroPage, 2, 5);
	_opCodeMap[0x76] = std::make_tuple(&atre::CPU::opROR, Addressing::ZeroPageX, 2, 6);
	_opCodeMap[0x6E] = std::make_tuple(&atre::CPU::opROR, Addressing::Absolute, 3, 6);
	_opCodeMap[0x7E] = std::make_tuple(&atre::CPU::opROR, Addressing::AbsoluteX, 3, 7);

	// RTI
	_opCodeMap[0x40] = std::make_tuple(&atre::CPU::opRTI, Addressing::None, 1, 6);

	// SBC
	_opCodeMap[0xE9] = std::make_tuple(&atre::CPU::opSBC, Addressing::Immediate, 2, 2);
	_opCodeMap[0xE5] = std::make_tuple(&atre::CPU::opSBC, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0xF5] = std::make_tuple(&atre::CPU::opSBC, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0xED] = std::make_tuple(&atre::CPU::opSBC, Addressing::Absolute, 3, 4);
	_opCodeMap[0xFD] = std::make_tuple(&atre::CPU::opSBC, Addressing::AbsoluteXPaged, 3, 4);
	_opCodeMap[0xF9] = std::make_tuple(&atre::CPU::opSBC, Addressing::AbsoluteYPaged, 3, 4);
	_opCodeMap[0xE1] = std::make_tuple(&atre::CPU::opSBC, Addressing::IndexedIndirect, 2, 6);
	_opCodeMap[0xF1] = std::make_tuple(&atre::CPU::opSBC, Addressing::IndirectIndexedPaged, 2, 5);

	// set flags
	_opCodeMap[0x38] = std::make_tuple(&atre::CPU::opSEC, Addressing::None, 1, 2);
	_opCodeMap[0xF8] = std::make_tuple(&atre::CPU::opSED, Addressing::None, 1, 2);
	_opCodeMap[0x78] = std::make_tuple(&atre::CPU::opSEI, Addressing::None, 1, 2);

	// store
	_opCodeMap[0x85] = std::make_tuple(&atre::CPU::opSTA, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0x95] = std::make_tuple(&atre::CPU::opSTA, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0x8D] = std::make_tuple(&atre::CPU::opSTA, Addressing::Absolute, 3, 4);
	_opCodeMap[0x9D] = std::make_tuple(&atre::CPU::opSTA, Addressing::AbsoluteX, 3, 5);
	_opCodeMap[0x99] = std::make_tuple(&atre::CPU::opSTA, Addressing::AbsoluteY, 3, 5);
	_opCodeMap[0x81] = std::make_tuple(&atre::CPU::opSTA, Addressing::IndexedIndirect, 2, 6);
	_opCodeMap[0x91] = std::make_tuple(&atre::CPU::opSTA, Addressing::IndirectIndexed, 2, 6);
	_opCodeMap[0x86] = std::make_tuple(&atre::CPU::opSTX, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0x96] = std::make_tuple(&atre::CPU::opSTX, Addressing::ZeroPageY, 2, 4);
	_opCodeMap[0x8E] = std::make_tuple(&atre::CPU::opSTX, Addressing::Absolute, 3, 4);
	_opCodeMap[0x84] = std::make_tuple(&atre::CPU::opSTY, Addressing::ZeroPage, 2, 3);
	_opCodeMap[0x94] = std::make_tuple(&atre::CPU::opSTY, Addressing::ZeroPageX, 2, 4);
	_opCodeMap[0x8C] = std::make_tuple(&atre::CPU::opSTY, Addressing::Absolute, 3, 4);

	// transfer
	_opCodeMap[0xAA] = std::make_tuple(&atre::CPU::opTAX, Addressing::None, 1, 2);
	_opCodeMap[0xA8] = std::make_tuple(&atre::CPU::opTAY, Addressing::None, 1, 2);
	_opCodeMap[0xBA] = std::make_tuple(&atre::CPU::opTSX, Addressing::None, 1, 2);
	_opCodeMap[0x8A] = std::make_tuple(&atre::CPU::opTXA, Addressing::None, 1, 2);
	_opCodeMap[0x9A] = std::make_tuple(&atre::CPU::opTXS, Addressing::None, 1, 2);
	_opCodeMap[0x98] = std::make_tuple(&atre::CPU::opTYA, Addressing::None, 1, 2);
}

void CPU::JumpTo(word_t startAddr)
{
	PC = startAddr;
	EC = startAddr;
}

void CPU::ExecuteUntil(word_t endAddr)
{
	EC = endAddr;
	for (;;)
	{
		Execute();
		if (PC == EC)
		{
			return;
		}
	}
}

void CPU::Dump()
{
	std::cout << "A = " << std::hex << std::showbase << (int)A << " (" << std::dec << (int)A << "), ";
	std::cout << "X = " << std::hex << std::showbase << (int)X << " (" << std::dec << (int)X << "), ";
	std::cout << "Y = " << std::hex << std::showbase << (int)Y << " (" << std::dec << (int)Y << "), ";
	std::cout << "PC = " << std::hex << std::showbase << (int)PC << ", ";
	std::cout << "S = " << std::hex << std::showbase << (int)S << ", ";
	std::cout << "Flags = ";
	std::cout << (IsSetFlag(CPU::NEGATIVE_FLAG) ? "N" : "n");
	std::cout << (IsSetFlag(CPU::OVERFLOW_FLAG) ? "O" : "o");
	std::cout << (IsSetFlag(CPU::IGNORED_FLAG) ? "X" : "x");
	std::cout << (IsSetFlag(CPU::BREAK_FLAG) ? "B" : "b");
	std::cout << (IsSetFlag(CPU::DECIMAL_FLAG) ? "D" : "d");
	std::cout << (IsSetFlag(CPU::INTERRUPT_FLAG) ? "I" : "i");
	std::cout << (IsSetFlag(CPU::ZERO_FLAG) ? "Z" : "z");
	std::cout << (IsSetFlag(CPU::CARRY_FLAG) ? "C" : "c");
	std::cout << " " << std::bitset<8>(F) << std::endl;
}

void CPU::Execute()
{
	if (_nmiPending)
	{
		NMI();
		_nmiPending = false;
		return;
	}
	if (_irqPending && !CPU::IsSetFlag(CPU::INTERRUPT_FLAG))
	{
		IRQ();
		return;
	}
	byte_t code = mMemory->Get(PC);
	auto &opCode = _opCodeMap[code];
	if (std::get<2>(opCode) > 0)
	{
		auto adr = std::get<1>(opCode);
		auto func = std::get<0>(opCode);
		auto opIndex = PC + 1;

		// PC already points to next instruction
		PC += std::get<2>(opCode);
		(this->*func)(opIndex, adr);

		if (mDumpState)
		{
			Dump();
		}

		if (PC == opIndex - 1 && mEnableTraps)
		{
			// jump to self: trap
			throw std::runtime_error("Trap!");
		}

		Cycles(std::get<3>(opCode));
	}
	else
	{
		throw std::runtime_error("Unsupported OPcode");
	}
}

unsigned long CPU::Cycles() const
{
	return (_seconds * CYCLES_PER_SEC) + _cycles;
}

} // namespace atre
