#include "CPU.hpp"
#include "Chips.hpp"
#include "IO.hpp"

using namespace std;

namespace atre
{
CPU::CPU(RAM* ram) :
	m_showCycles(), m_enableTraps(), m_showSteps(), m_callStack(), m_opCodeMap(), A(), X(), Y(), S(), PC(), F(), BRK(),
	m_cycles(0), m_seconds(0), m_irqPending(), m_nmiPending(), m_waitCycles(0), m_RAM(ram), m_callbacks(), m_IO()
{
	InitializeOPCodes();
}

void CPU::Attach(Callbacks* callbacks)
{
	m_callbacks = callbacks;
}

void CPU::Connect(IO* io)
{
	m_IO = io;
}

void CPU::Wait(unsigned cycles)
{
	m_waitCycles = cycles;
}

void CPU::Reset()
{
	if(m_callbacks)
	{
		m_callbacks->OnReset();
	}
	A = X = Y	 = 0;
	F			 = CPU::IGNORED_FLAG;
	S			 = 0xFF;
	PC			 = m_RAM->GetW(0xFFFC);
	BRK			 = 0xFFFC;
	m_cycles	 = 0;
	m_seconds	 = 0;
	m_irqPending = false;
	m_nmiPending = false;
	m_callStack.clear();
	m_callStack.push_back(make_pair(PC, "Entry"));
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
	if(isSet)
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
	m_RAM->Set(0x100 + S, val);
	S--;
}

byte_t CPU::StackPull()
{
	S++;
	return m_RAM->Get(0x100 + S);
}

void CPU::Cycles(unsigned long cycles)
{
	m_cycles += cycles;
	if(m_IO)
	{
		while(cycles-- > 0)
		{
			m_IO->Tick();
		}
	}
	if(m_cycles >= CYCLES_PER_SEC)
	{
		m_seconds++;
		m_cycles -= CYCLES_PER_SEC;
		if(m_showCycles)
		{
			cout << "#" << flush;
		}
	}
}

byte_t CPU::GetOP(word_t opIndex, AddressingMode adr)
{
	switch(adr)
	{
	case AddressingMode::Accumulator:
	{
		return A;
	}
	case AddressingMode::Immediate:
	{
		return m_RAM->Get(opIndex);
	}
	case AddressingMode::Absolute:
	{
		word_t addr = m_RAM->GetW(opIndex);
		return m_RAM->Get(addr);
	}
	case AddressingMode::ZeroPage:
	{
		return m_RAM->Get(m_RAM->Get(opIndex));
	}
	case AddressingMode::ZeroPageX:
	{
		return m_RAM->Get((m_RAM->Get(opIndex) + X) & 0xFF);
	}
	case AddressingMode::ZeroPageY:
	{
		return m_RAM->Get((m_RAM->Get(opIndex) + Y) & 0xFF);
	}
	case AddressingMode::AbsoluteX:
	case AddressingMode::AbsoluteXPaged:
	{
		word_t baseAddr	 = m_RAM->GetW(opIndex);
		auto   finalAddr = static_cast<word_t>(baseAddr + X);
		if(adr == AddressingMode::AbsoluteXPaged && baseAddr >> 8 != finalAddr >> 8)
		{
			Cycles(1);
		}
		return m_RAM->Get(finalAddr);
	}
	case AddressingMode::AbsoluteY:
	case AddressingMode::AbsoluteYPaged:
	{
		word_t baseAddr	 = m_RAM->GetW(opIndex);
		auto   finalAddr = static_cast<word_t>(baseAddr + Y);
		if(adr == AddressingMode::AbsoluteYPaged && baseAddr >> 8 != finalAddr >> 8)
		{
			Cycles(1);
		}
		return m_RAM->Get(finalAddr);
	}
	case AddressingMode::IndexedIndirect:
	{
		word_t baseAddr = (m_RAM->Get(opIndex) + X) & 0xFF;
		word_t loTarget = m_RAM->Get(baseAddr);
		word_t hiTarget = m_RAM->Get((baseAddr + 1) & 0xFF);
		word_t addr		= loTarget + (hiTarget << 8);
		return m_RAM->Get(addr);
	}
	case AddressingMode::IndirectIndexed:
	case AddressingMode::IndirectIndexedPaged:
	{
		byte_t basePointer	= m_RAM->Get(opIndex);
		word_t loTarget		= m_RAM->Get(basePointer);
		word_t hiTarget		= m_RAM->Get((basePointer + 1) & 0xFF);
		word_t baseAddress	= loTarget + (hiTarget << 8);
		word_t finalAddress = baseAddress + Y;
		if(adr == AddressingMode::IndirectIndexedPaged && baseAddress >> 8 != finalAddress >> 8)
		{
			Cycles(1);
		}
		return m_RAM->Get(finalAddress);
	}
	default:
		throw runtime_error("Not supported addressing mode");
	}
}

void CPU::SetOP(word_t opIndex, AddressingMode adr, byte_t val)
{
	switch(adr)
	{
	case AddressingMode::Accumulator:
	{
		A = val;
		break;
	}
	case AddressingMode::Immediate:
	{
		m_RAM->Set(opIndex, val);
		break;
	}
	case AddressingMode::Absolute:
	{
		word_t addr = m_RAM->GetW(opIndex);
		m_RAM->Set(addr, val);
		break;
	}
	case AddressingMode::ZeroPage:
	{
		m_RAM->Set(m_RAM->Get(opIndex), val);
		break;
	}
	case AddressingMode::ZeroPageX:
	{
		m_RAM->Set((m_RAM->Get(opIndex) + X) & 0xFF, val);
		break;
	}
	case AddressingMode::ZeroPageY:
	{
		m_RAM->Set((m_RAM->Get(opIndex) + Y) & 0xFF, val);
		break;
	}
	case AddressingMode::AbsoluteX:
	{
		word_t baseAddr	 = m_RAM->GetW(opIndex);
		auto   finalAddr = static_cast<word_t>(baseAddr + X);
		m_RAM->Set(finalAddr, val);
		break;
	}
	case AddressingMode::AbsoluteY:
	{
		word_t baseAddr	 = m_RAM->GetW(opIndex);
		auto   finalAddr = static_cast<word_t>(baseAddr + Y);
		m_RAM->Set(finalAddr, val);
		break;
	}
	case AddressingMode::IndexedIndirect:
	{
		word_t baseAddr = (m_RAM->Get(opIndex) + X) & 0xFF;
		word_t loTarget = m_RAM->Get(baseAddr);
		word_t hiTarget = m_RAM->Get((baseAddr + 1) & 0xFF);
		word_t addr		= loTarget + (hiTarget << 8);
		m_RAM->Set(addr, val);
		break;
	}
	case AddressingMode::IndirectIndexed:
	{
		byte_t basePointer	= m_RAM->Get(opIndex);
		word_t loTarget		= m_RAM->Get(basePointer);
		word_t hiTarget		= m_RAM->Get((basePointer + 1) & 0xFF);
		word_t baseAddress	= loTarget + (hiTarget << 8);
		word_t finalAddress = baseAddress + Y;
		m_RAM->Set(finalAddress, val);
		break;
	}
	default:
		throw runtime_error("Not supported addressing mode");
	}
}

void CPU::ADC(byte_t op)
{
	if(IsSetFlag(CPU::DECIMAL_FLAG))
	{
		auto lhr = (A & 15) + (A >> 4) * 10;
		auto rhr = (op & 15) + (op >> 4) * 10;
		auto res = lhr + rhr + (IsSetFlag(CPU::CARRY_FLAG) ? 1 : 0);
		SetFlag(CPU::CARRY_FLAG, res > 99);
		res %= 100;
		A = static_cast<byte_t>(((res / 10) << 4) + (res % 10));
		SetFlag(CPU::NEGATIVE_FLAG, IsNegative(static_cast<byte_t>(res)));
		SetFlag(CPU::ZERO_FLAG, IsZero(static_cast<byte_t>(res)));
	}
	else
	{
		word_t wresult = A + op + (IsSetFlag(CPU::CARRY_FLAG) ? 1 : 0);
		byte_t res	   = (byte_t)wresult;
		SetFlag(CPU::CARRY_FLAG, wresult > 255);
		SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
		SetFlag(CPU::ZERO_FLAG, IsZero(res));
		SetFlag(CPU::OVERFLOW_FLAG,
				IsNegative(static_cast<byte_t>(A ^ res)) & IsNegative(static_cast<byte_t>(op ^ res)));
		A = res;
	}
}

void CPU::opADC(word_t opIndex, AddressingMode adr)
{
	ADC(GetOP(opIndex, adr));
}

void CPU::AND(byte_t op)
{
	A &= op;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opAND(word_t opIndex, AddressingMode adr)
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
void CPU::opASL(word_t opIndex, AddressingMode adr)
{
	SetOP(opIndex, adr, ASL(GetOP(opIndex, adr)));
}

void CPU::Branch(word_t opIndex, bool condition)
{
	if(condition)
	{
		Cycles(1);
		const auto target = static_cast<word_t>(PC + static_cast<sbyte_t>(m_RAM->Get(opIndex)));
		if(PC >> 8 != target >> 8)
		{
			Cycles(1);
		}
		PC = target;
	}
}

// Branch on Carry Clear
void CPU::opBCC(word_t opIndex, AddressingMode /*adr*/)
{
	Branch(opIndex, !IsSetFlag(CPU::CARRY_FLAG));
}

// Branch on Carry Set
void CPU::opBCS(word_t opIndex, AddressingMode /*adr*/)
{
	Branch(opIndex, IsSetFlag(CPU::CARRY_FLAG));
}

// Branch on Result Zero
void CPU::opBEQ(word_t opIndex, AddressingMode /*adr*/)
{
	Branch(opIndex, IsSetFlag(CPU::ZERO_FLAG));
}

// Branch on Result Minus
void CPU::opBMI(word_t opIndex, AddressingMode /*adr*/)
{
	Branch(opIndex, IsSetFlag(CPU::NEGATIVE_FLAG));
}

// Branch on Result not Zero
void CPU::opBNE(word_t opIndex, AddressingMode /*adr*/)
{
	Branch(opIndex, !IsSetFlag(CPU::ZERO_FLAG));
}

// Branch on Result Plus
void CPU::opBPL(word_t opIndex, AddressingMode /*adr*/)
{
	Branch(opIndex, !IsSetFlag(CPU::NEGATIVE_FLAG));
}

// Branch on Overflow Clear
void CPU::opBVC(word_t opIndex, AddressingMode /*adr*/)
{
	Branch(opIndex, !IsSetFlag(CPU::OVERFLOW_FLAG));
}

// Branch on Overflow Set
void CPU::opBVS(word_t opIndex, AddressingMode /*adr*/)
{
	Branch(opIndex, IsSetFlag(CPU::OVERFLOW_FLAG));
}

// Test Bits in getRAM with Accumulator
void CPU::opBIT(word_t opIndex, AddressingMode adr)
{
	byte_t op = GetOP(opIndex, adr);
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(op));
	SetFlag(CPU::OVERFLOW_FLAG, op & 0b01000000);
	byte_t res = (A & op);
	SetFlag(CPU::ZERO_FLAG, IsZero(res));
}

void CPU::IRQ()
{
	m_irqPending = true;
}

void CPU::NMI()
{
	m_nmiPending = true;
}

// external interrupt
void CPU::doIRQ()
{
	if(m_callbacks)
	{
		m_callbacks->OnIRQ();
	}
	StackPush(PC >> 8);
	StackPush(PC & 0xFF);
	StackPush(F);
	SetFlag(CPU::INTERRUPT_FLAG);
	PC = m_RAM->GetW(0xFFFE); // jump to vector
	m_callStack.push_back(make_pair(PC, "IRQ"));
}

// non-masked interrupt
void CPU::doNMI()
{
	if(m_callbacks)
	{
		m_callbacks->OnNMI();
	}
	StackPush(PC >> 8);
	StackPush(PC & 0xFF);
	StackPush(F);
	SetFlag(CPU::INTERRUPT_FLAG);
	PC = m_RAM->GetW(0xFFFA); // jump to vector
	m_callStack.push_back(make_pair(PC, "NMI"));
}

// Force Break
void CPU::opBRK(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	if(m_callbacks)
	{
		m_callbacks->OnBRK();
	}
	PC++;
	StackPush(PC >> 8);
	StackPush(PC & 0xFF);
	auto f = F;
	f |= BREAK_FLAG;
	StackPush(f);
	SetFlag(CPU::INTERRUPT_FLAG);
	PC = m_RAM->GetW(0xFFFE); // jump to vector
	m_callStack.push_back(make_pair(PC, "BRK"));
}

void CPU::opCLC(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	ClearFlag(CPU::CARRY_FLAG);
}

void CPU::opCLD(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	ClearFlag(CPU::DECIMAL_FLAG);
}

void CPU::opCLI(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	ClearFlag(CPU::INTERRUPT_FLAG);
}

void CPU::opCLV(word_t /*opIndex*/, AddressingMode /*adr*/)
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

void CPU::opCMP(word_t opIndex, AddressingMode adr)
{
	Compare(A, GetOP(opIndex, adr));
}

void CPU::opCPX(word_t opIndex, AddressingMode adr)
{
	Compare(X, GetOP(opIndex, adr));
}

void CPU::opCPY(word_t opIndex, AddressingMode adr)
{
	Compare(Y, GetOP(opIndex, adr));
}

void CPU::opDEC(word_t opIndex, AddressingMode adr)
{
	auto res = GetOP(opIndex, adr);
	res--;
	SetFlag(CPU::ZERO_FLAG, IsZero(res));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
	SetOP(opIndex, adr, res);
}

void CPU::opDEX(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	X--;
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
}

void CPU::opDEY(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	Y--;
	SetFlag(CPU::ZERO_FLAG, IsZero(Y));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(Y));
}

void CPU::opINC(word_t opIndex, AddressingMode adr)
{
	auto res = GetOP(opIndex, adr);
	res++;
	SetFlag(CPU::ZERO_FLAG, IsZero(res));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
	SetOP(opIndex, adr, res);
}

void CPU::opINX(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	X++;
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
}

void CPU::opINY(word_t /*opIndex*/, AddressingMode /*adr*/)
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

void CPU::opEOR(word_t opIndex, AddressingMode adr)
{
	EOR(GetOP(opIndex, adr));
}

void CPU::opJMP(word_t opIndex, AddressingMode adr)
{
	switch(adr)
	{
	case AddressingMode::Absolute:
	{
		PC = m_RAM->GetW(opIndex);
		break;
	}
	case AddressingMode::Indirect:
	{
		auto addr = m_RAM->GetW(opIndex);
		PC		  = m_RAM->GetW(addr);
		break;
	}
	default:
		throw runtime_error("Not supported addressing mode");
	}
}

void CPU::opJSR(word_t opIndex, AddressingMode /*adr*/)
{
	const auto retAddress = static_cast<word_t>(PC - 1); // next instruction - 1
	StackPush(retAddress >> 8);
	StackPush(retAddress & 0xFF);
	PC = m_RAM->GetW(opIndex);
	m_callStack.push_back(make_pair(PC, "JSR"));
}

void CPU::opRTS(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	if(m_callStack.size())
	{
		m_callStack.pop_back();
	}
	word_t retAddress = StackPull();
	retAddress += (StackPull() << 8);
	retAddress++; // we pushed next instruction - 1
	PC = retAddress;
}

void CPU::opLDA(word_t opIndex, AddressingMode adr)
{
	A = GetOP(opIndex, adr);
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opLDX(word_t opIndex, AddressingMode adr)
{
	X = GetOP(opIndex, adr);
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
}

void CPU::opLDY(word_t opIndex, AddressingMode adr)
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

void CPU::opLSR(word_t opIndex, AddressingMode adr)
{
	SetOP(opIndex, adr, LSR(GetOP(opIndex, adr)));
}

void CPU::opNOP(word_t /*opIndex*/, AddressingMode /*adr*/) {}

void CPU::ORA(byte_t op)
{
	A |= op;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opORA(word_t opIndex, AddressingMode adr)
{
	ORA(GetOP(opIndex, adr));
}

void CPU::opPHA(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	StackPush(A);
}

void CPU::opPHP(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	auto f = F;
	f |= CPU::BREAK_FLAG;
	StackPush(f);
}

void CPU::opPLA(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	A = StackPull();
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opPLP(word_t /*opIndex*/, AddressingMode /*adr*/)
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
	if(hadCarry)
	{
		op++;
	}
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(op));
	SetFlag(CPU::ZERO_FLAG, IsZero(op));
	return op;
}

void CPU::opROL(word_t opIndex, AddressingMode adr)
{
	SetOP(opIndex, adr, ROL(GetOP(opIndex, adr)));
}

byte_t CPU::ROR(byte_t op)
{
	auto hadCarry = IsSetFlag(CPU::CARRY_FLAG);
	SetFlag(CPU::CARRY_FLAG, op & 0x01);
	op >>= 1;
	if(hadCarry)
	{
		op += 0x80;
	}
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(op));
	SetFlag(CPU::ZERO_FLAG, IsZero(op));
	return op;
}

void CPU::opROR(word_t opIndex, AddressingMode adr)
{
	SetOP(opIndex, adr, ROR(GetOP(opIndex, adr)));
}

void CPU::opRTI(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	if(m_callStack.size())
	{
		m_callStack.pop_back();
	}
	F = StackPull();
	// ClearFlag(getCPU::INTERRUPT_FLAG);
	word_t retAddress = StackPull();
	retAddress += (StackPull() << 8);
	PC = retAddress;
}

void CPU::SBC(byte_t op)
{
	if(IsSetFlag(CPU::DECIMAL_FLAG))
	{
		auto lhr = (A & 15) + (A >> 4) * 10;
		auto rhr = (op & 15) + (op >> 4) * 10;
		auto res = lhr - rhr - (IsSetFlag(CPU::CARRY_FLAG) ? 0 : 1);
		SetFlag(CPU::CARRY_FLAG, res >= 0);
		if(res < 0)
		{
			res += 100;
		}
		A = static_cast<byte_t>(((res / 10) << 4) + (res % 10));
		SetFlag(CPU::NEGATIVE_FLAG, IsNegative(static_cast<byte_t>(res)));
		SetFlag(CPU::ZERO_FLAG, IsZero(static_cast<byte_t>(res)));
	}
	else
	{
		ADC(op ^ 0xFF);
	}
}

void CPU::opSBC(word_t opIndex, AddressingMode adr)
{
	SBC(GetOP(opIndex, adr));
}

void CPU::opSEC(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	SetFlag(CPU::CARRY_FLAG);
}

void CPU::opSED(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	SetFlag(CPU::DECIMAL_FLAG);
}

void CPU::opSEI(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	SetFlag(CPU::INTERRUPT_FLAG);
}

void CPU::opSTA(word_t opIndex, AddressingMode adr)
{
	SetOP(opIndex, adr, A);
}

void CPU::opSTX(word_t opIndex, AddressingMode adr)
{
	SetOP(opIndex, adr, X);
}

void CPU::opSTY(word_t opIndex, AddressingMode adr)
{
	SetOP(opIndex, adr, Y);
}

void CPU::opTAX(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	X = A;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
}

void CPU::opTAY(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	Y = A;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(Y));
	SetFlag(CPU::ZERO_FLAG, IsZero(Y));
}

void CPU::opTSX(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	X = S;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(X));
	SetFlag(CPU::ZERO_FLAG, IsZero(X));
}

void CPU::opTXA(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	A = X;
	SetFlag(CPU::NEGATIVE_FLAG, IsNegative(A));
	SetFlag(CPU::ZERO_FLAG, IsZero(A));
}

void CPU::opTXS(word_t /*opIndex*/, AddressingMode /*adr*/)
{
	S = X;
}

void CPU::opTYA(word_t /*opIndex*/, AddressingMode /*adr*/)
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
	m_opCodeMap[0x69] = make_tuple(&atre::CPU::opADC, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0x65] = make_tuple(&atre::CPU::opADC, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0x75] = make_tuple(&atre::CPU::opADC, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0x6D] = make_tuple(&atre::CPU::opADC, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0x7D] = make_tuple(&atre::CPU::opADC, AddressingMode::AbsoluteXPaged, 3, 4);
	m_opCodeMap[0x79] = make_tuple(&atre::CPU::opADC, AddressingMode::AbsoluteYPaged, 3, 4);
	m_opCodeMap[0x61] = make_tuple(&atre::CPU::opADC, AddressingMode::IndexedIndirect, 2, 6);
	m_opCodeMap[0x71] = make_tuple(&atre::CPU::opADC, AddressingMode::IndirectIndexedPaged, 2, 5);

	// AND
	m_opCodeMap[0x29] = make_tuple(&atre::CPU::opAND, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0x25] = make_tuple(&atre::CPU::opAND, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0x35] = make_tuple(&atre::CPU::opAND, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0x2D] = make_tuple(&atre::CPU::opAND, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0x3D] = make_tuple(&atre::CPU::opAND, AddressingMode::AbsoluteXPaged, 3, 4);
	m_opCodeMap[0x39] = make_tuple(&atre::CPU::opAND, AddressingMode::AbsoluteYPaged, 3, 4);
	m_opCodeMap[0x21] = make_tuple(&atre::CPU::opAND, AddressingMode::IndexedIndirect, 2, 6);
	m_opCodeMap[0x31] = make_tuple(&atre::CPU::opAND, AddressingMode::IndirectIndexedPaged, 2, 5);

	// ASL
	m_opCodeMap[0x0A] = make_tuple(&atre::CPU::opASL, AddressingMode::Accumulator, 1, 2);
	m_opCodeMap[0x06] = make_tuple(&atre::CPU::opASL, AddressingMode::ZeroPage, 2, 5);
	m_opCodeMap[0x16] = make_tuple(&atre::CPU::opASL, AddressingMode::ZeroPageX, 2, 6);
	m_opCodeMap[0x0E] = make_tuple(&atre::CPU::opASL, AddressingMode::Absolute, 3, 6);
	m_opCodeMap[0x1E] = make_tuple(&atre::CPU::opASL, AddressingMode::AbsoluteX, 3, 7);

	// branching
	m_opCodeMap[0x90] = make_tuple(&atre::CPU::opBCC, AddressingMode::Relative, 2, 2);
	m_opCodeMap[0xB0] = make_tuple(&atre::CPU::opBCS, AddressingMode::Relative, 2, 2);
	m_opCodeMap[0xF0] = make_tuple(&atre::CPU::opBEQ, AddressingMode::Relative, 2, 2);
	m_opCodeMap[0x30] = make_tuple(&atre::CPU::opBMI, AddressingMode::Relative, 2, 2);
	m_opCodeMap[0xD0] = make_tuple(&atre::CPU::opBNE, AddressingMode::Relative, 2, 2);
	m_opCodeMap[0x10] = make_tuple(&atre::CPU::opBPL, AddressingMode::Relative, 2, 2);
	m_opCodeMap[0x50] = make_tuple(&atre::CPU::opBVC, AddressingMode::Relative, 2, 2);
	m_opCodeMap[0x70] = make_tuple(&atre::CPU::opBVS, AddressingMode::Relative, 2, 2);

	// BIT
	m_opCodeMap[0x24] = make_tuple(&atre::CPU::opBIT, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0x2C] = make_tuple(&atre::CPU::opBIT, AddressingMode::Absolute, 3, 4);

	// BRK
	m_opCodeMap[0x00] = make_tuple(&atre::CPU::opBRK, AddressingMode::None, 1, 7);

	// clear flags
	m_opCodeMap[0x18] = make_tuple(&atre::CPU::opCLC, AddressingMode::None, 1, 2);
	m_opCodeMap[0xD8] = make_tuple(&atre::CPU::opCLD, AddressingMode::None, 1, 2);
	m_opCodeMap[0x58] = make_tuple(&atre::CPU::opCLI, AddressingMode::None, 1, 2);
	m_opCodeMap[0xB8] = make_tuple(&atre::CPU::opCLV, AddressingMode::None, 1, 2);

	// comparisons
	m_opCodeMap[0xC9] = make_tuple(&atre::CPU::opCMP, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0xC5] = make_tuple(&atre::CPU::opCMP, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0xD5] = make_tuple(&atre::CPU::opCMP, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0xCD] = make_tuple(&atre::CPU::opCMP, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0xDD] = make_tuple(&atre::CPU::opCMP, AddressingMode::AbsoluteXPaged, 3, 4);
	m_opCodeMap[0xD9] = make_tuple(&atre::CPU::opCMP, AddressingMode::AbsoluteYPaged, 3, 4);
	m_opCodeMap[0xC1] = make_tuple(&atre::CPU::opCMP, AddressingMode::IndexedIndirect, 2, 6);
	m_opCodeMap[0xD1] = make_tuple(&atre::CPU::opCMP, AddressingMode::IndirectIndexedPaged, 2, 5);
	m_opCodeMap[0xE0] = make_tuple(&atre::CPU::opCPX, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0xE4] = make_tuple(&atre::CPU::opCPX, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0xEC] = make_tuple(&atre::CPU::opCPX, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0xC0] = make_tuple(&atre::CPU::opCPY, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0xC4] = make_tuple(&atre::CPU::opCPY, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0xCC] = make_tuple(&atre::CPU::opCPY, AddressingMode::Absolute, 3, 4);

	// increment/decrement
	m_opCodeMap[0xE6] = make_tuple(&atre::CPU::opINC, AddressingMode::ZeroPage, 2, 5);
	m_opCodeMap[0xF6] = make_tuple(&atre::CPU::opINC, AddressingMode::ZeroPageX, 2, 6);
	m_opCodeMap[0xEE] = make_tuple(&atre::CPU::opINC, AddressingMode::Absolute, 3, 6);
	m_opCodeMap[0xFE] = make_tuple(&atre::CPU::opINC, AddressingMode::AbsoluteX, 3, 7);
	m_opCodeMap[0xE8] = make_tuple(&atre::CPU::opINX, AddressingMode::None, 1, 2);
	m_opCodeMap[0xC8] = make_tuple(&atre::CPU::opINY, AddressingMode::None, 1, 2);
	m_opCodeMap[0xC6] = make_tuple(&atre::CPU::opDEC, AddressingMode::ZeroPage, 2, 5);
	m_opCodeMap[0xD6] = make_tuple(&atre::CPU::opDEC, AddressingMode::ZeroPageX, 2, 6);
	m_opCodeMap[0xCE] = make_tuple(&atre::CPU::opDEC, AddressingMode::Absolute, 3, 6);
	m_opCodeMap[0xDE] = make_tuple(&atre::CPU::opDEC, AddressingMode::AbsoluteX, 3, 7);
	m_opCodeMap[0xCA] = make_tuple(&atre::CPU::opDEX, AddressingMode::None, 1, 2);
	m_opCodeMap[0x88] = make_tuple(&atre::CPU::opDEY, AddressingMode::None, 1, 2);

	// EOR
	m_opCodeMap[0x49] = make_tuple(&atre::CPU::opEOR, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0x45] = make_tuple(&atre::CPU::opEOR, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0x55] = make_tuple(&atre::CPU::opEOR, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0x4D] = make_tuple(&atre::CPU::opEOR, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0x5D] = make_tuple(&atre::CPU::opEOR, AddressingMode::AbsoluteXPaged, 3, 4);
	m_opCodeMap[0x59] = make_tuple(&atre::CPU::opEOR, AddressingMode::AbsoluteYPaged, 3, 4);
	m_opCodeMap[0x41] = make_tuple(&atre::CPU::opEOR, AddressingMode::IndexedIndirect, 2, 6);
	m_opCodeMap[0x51] = make_tuple(&atre::CPU::opEOR, AddressingMode::IndirectIndexedPaged, 2, 5);

	// JMP
	m_opCodeMap[0x4C] = make_tuple(&atre::CPU::opJMP, AddressingMode::Absolute, 3, 3);
	m_opCodeMap[0x6C] = make_tuple(&atre::CPU::opJMP, AddressingMode::Indirect, 3, 5);

	// subroutine
	m_opCodeMap[0x20] = make_tuple(&atre::CPU::opJSR, AddressingMode::Absolute, 3, 6);
	m_opCodeMap[0x60] = make_tuple(&atre::CPU::opRTS, AddressingMode::Absolute, 1, 6);

	// load
	m_opCodeMap[0xA9] = make_tuple(&atre::CPU::opLDA, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0xA5] = make_tuple(&atre::CPU::opLDA, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0xB5] = make_tuple(&atre::CPU::opLDA, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0xAD] = make_tuple(&atre::CPU::opLDA, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0xBD] = make_tuple(&atre::CPU::opLDA, AddressingMode::AbsoluteXPaged, 3, 4);
	m_opCodeMap[0xB9] = make_tuple(&atre::CPU::opLDA, AddressingMode::AbsoluteYPaged, 3, 4);
	m_opCodeMap[0xA1] = make_tuple(&atre::CPU::opLDA, AddressingMode::IndexedIndirect, 2, 6);
	m_opCodeMap[0xB1] = make_tuple(&atre::CPU::opLDA, AddressingMode::IndirectIndexedPaged, 2, 5);
	m_opCodeMap[0xA2] = make_tuple(&atre::CPU::opLDX, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0xA6] = make_tuple(&atre::CPU::opLDX, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0xB6] = make_tuple(&atre::CPU::opLDX, AddressingMode::ZeroPageY, 2, 4);
	m_opCodeMap[0xAE] = make_tuple(&atre::CPU::opLDX, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0xBE] = make_tuple(&atre::CPU::opLDX, AddressingMode::AbsoluteYPaged, 3, 4);
	m_opCodeMap[0xA0] = make_tuple(&atre::CPU::opLDY, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0xA4] = make_tuple(&atre::CPU::opLDY, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0xB4] = make_tuple(&atre::CPU::opLDY, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0xAC] = make_tuple(&atre::CPU::opLDY, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0xBC] = make_tuple(&atre::CPU::opLDY, AddressingMode::AbsoluteXPaged, 3, 4);

	// LSR
	m_opCodeMap[0x4A] = make_tuple(&atre::CPU::opLSR, AddressingMode::Accumulator, 1, 2);
	m_opCodeMap[0x46] = make_tuple(&atre::CPU::opLSR, AddressingMode::ZeroPage, 2, 5);
	m_opCodeMap[0x56] = make_tuple(&atre::CPU::opLSR, AddressingMode::ZeroPageX, 2, 6);
	m_opCodeMap[0x4E] = make_tuple(&atre::CPU::opLSR, AddressingMode::Absolute, 3, 6);
	m_opCodeMap[0x5E] = make_tuple(&atre::CPU::opLSR, AddressingMode::AbsoluteX, 3, 7);

	// NOP
	m_opCodeMap[0xEA] = make_tuple(&atre::CPU::opNOP, AddressingMode::None, 1, 2);

	// ORA
	m_opCodeMap[0x09] = make_tuple(&atre::CPU::opORA, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0x05] = make_tuple(&atre::CPU::opORA, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0x15] = make_tuple(&atre::CPU::opORA, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0x0D] = make_tuple(&atre::CPU::opORA, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0x1D] = make_tuple(&atre::CPU::opORA, AddressingMode::AbsoluteXPaged, 3, 4);
	m_opCodeMap[0x19] = make_tuple(&atre::CPU::opORA, AddressingMode::AbsoluteYPaged, 3, 4);
	m_opCodeMap[0x01] = make_tuple(&atre::CPU::opORA, AddressingMode::IndexedIndirect, 2, 6);
	m_opCodeMap[0x11] = make_tuple(&atre::CPU::opORA, AddressingMode::IndirectIndexedPaged, 2, 5);

	// push/pull
	m_opCodeMap[0x48] = make_tuple(&atre::CPU::opPHA, AddressingMode::IndirectIndexed, 1, 3);
	m_opCodeMap[0x08] = make_tuple(&atre::CPU::opPHP, AddressingMode::IndirectIndexed, 1, 3);
	m_opCodeMap[0x68] = make_tuple(&atre::CPU::opPLA, AddressingMode::IndirectIndexed, 1, 4);
	m_opCodeMap[0x28] = make_tuple(&atre::CPU::opPLP, AddressingMode::IndirectIndexed, 1, 4);

	// ROL
	m_opCodeMap[0x2A] = make_tuple(&atre::CPU::opROL, AddressingMode::Accumulator, 1, 2);
	m_opCodeMap[0x26] = make_tuple(&atre::CPU::opROL, AddressingMode::ZeroPage, 2, 5);
	m_opCodeMap[0x36] = make_tuple(&atre::CPU::opROL, AddressingMode::ZeroPageX, 2, 6);
	m_opCodeMap[0x2E] = make_tuple(&atre::CPU::opROL, AddressingMode::Absolute, 3, 6);
	m_opCodeMap[0x3E] = make_tuple(&atre::CPU::opROL, AddressingMode::AbsoluteX, 3, 7);

	// ROR
	m_opCodeMap[0x6A] = make_tuple(&atre::CPU::opROR, AddressingMode::Accumulator, 1, 2);
	m_opCodeMap[0x66] = make_tuple(&atre::CPU::opROR, AddressingMode::ZeroPage, 2, 5);
	m_opCodeMap[0x76] = make_tuple(&atre::CPU::opROR, AddressingMode::ZeroPageX, 2, 6);
	m_opCodeMap[0x6E] = make_tuple(&atre::CPU::opROR, AddressingMode::Absolute, 3, 6);
	m_opCodeMap[0x7E] = make_tuple(&atre::CPU::opROR, AddressingMode::AbsoluteX, 3, 7);

	// RTI
	m_opCodeMap[0x40] = make_tuple(&atre::CPU::opRTI, AddressingMode::None, 1, 6);

	// SBC
	m_opCodeMap[0xE9] = make_tuple(&atre::CPU::opSBC, AddressingMode::Immediate, 2, 2);
	m_opCodeMap[0xE5] = make_tuple(&atre::CPU::opSBC, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0xF5] = make_tuple(&atre::CPU::opSBC, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0xED] = make_tuple(&atre::CPU::opSBC, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0xFD] = make_tuple(&atre::CPU::opSBC, AddressingMode::AbsoluteXPaged, 3, 4);
	m_opCodeMap[0xF9] = make_tuple(&atre::CPU::opSBC, AddressingMode::AbsoluteYPaged, 3, 4);
	m_opCodeMap[0xE1] = make_tuple(&atre::CPU::opSBC, AddressingMode::IndexedIndirect, 2, 6);
	m_opCodeMap[0xF1] = make_tuple(&atre::CPU::opSBC, AddressingMode::IndirectIndexedPaged, 2, 5);

	// set flags
	m_opCodeMap[0x38] = make_tuple(&atre::CPU::opSEC, AddressingMode::None, 1, 2);
	m_opCodeMap[0xF8] = make_tuple(&atre::CPU::opSED, AddressingMode::None, 1, 2);
	m_opCodeMap[0x78] = make_tuple(&atre::CPU::opSEI, AddressingMode::None, 1, 2);

	// store
	m_opCodeMap[0x85] = make_tuple(&atre::CPU::opSTA, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0x95] = make_tuple(&atre::CPU::opSTA, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0x8D] = make_tuple(&atre::CPU::opSTA, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0x9D] = make_tuple(&atre::CPU::opSTA, AddressingMode::AbsoluteX, 3, 5);
	m_opCodeMap[0x99] = make_tuple(&atre::CPU::opSTA, AddressingMode::AbsoluteY, 3, 5);
	m_opCodeMap[0x81] = make_tuple(&atre::CPU::opSTA, AddressingMode::IndexedIndirect, 2, 6);
	m_opCodeMap[0x91] = make_tuple(&atre::CPU::opSTA, AddressingMode::IndirectIndexed, 2, 6);
	m_opCodeMap[0x86] = make_tuple(&atre::CPU::opSTX, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0x96] = make_tuple(&atre::CPU::opSTX, AddressingMode::ZeroPageY, 2, 4);
	m_opCodeMap[0x8E] = make_tuple(&atre::CPU::opSTX, AddressingMode::Absolute, 3, 4);
	m_opCodeMap[0x84] = make_tuple(&atre::CPU::opSTY, AddressingMode::ZeroPage, 2, 3);
	m_opCodeMap[0x94] = make_tuple(&atre::CPU::opSTY, AddressingMode::ZeroPageX, 2, 4);
	m_opCodeMap[0x8C] = make_tuple(&atre::CPU::opSTY, AddressingMode::Absolute, 3, 4);

	// transfer
	m_opCodeMap[0xAA] = make_tuple(&atre::CPU::opTAX, AddressingMode::None, 1, 2);
	m_opCodeMap[0xA8] = make_tuple(&atre::CPU::opTAY, AddressingMode::None, 1, 2);
	m_opCodeMap[0xBA] = make_tuple(&atre::CPU::opTSX, AddressingMode::None, 1, 2);
	m_opCodeMap[0x8A] = make_tuple(&atre::CPU::opTXA, AddressingMode::None, 1, 2);
	m_opCodeMap[0x9A] = make_tuple(&atre::CPU::opTXS, AddressingMode::None, 1, 2);
	m_opCodeMap[0x98] = make_tuple(&atre::CPU::opTYA, AddressingMode::None, 1, 2);
}

void CPU::JumpTo(word_t startAddr)
{
	PC = startAddr;
}

void CPU::BreakAt(word_t breakAddr)
{
	BRK = breakAddr;
}

void CPU::Execute()
{
	if(m_waitCycles)
	{
		Cycles(1);
		m_waitCycles--;
		return;
	}
	if(m_nmiPending)
	{
		if(m_showSteps)
		{
			cout << "NMI" << endl;
		}
		doNMI();
		m_nmiPending = false;
		return;
	}
	if(m_irqPending && !CPU::IsSetFlag(CPU::INTERRUPT_FLAG))
	{
		if(m_showSteps)
		{
			cout << "IRQ" << endl;
		}
		m_irqPending = false;
		doIRQ();
		return;
	}
	const byte_t code	= m_RAM->Get(PC);
	const auto&	 opCode = m_opCodeMap[code];
	if(get<2>(opCode) > 0)
	{
		const auto adr	   = get<1>(opCode);
		const auto func	   = get<0>(opCode);
		const auto opIndex = static_cast<word_t>(PC + 1);

		// PC already points to next instruction
		PC += static_cast<word_t>(get<2>(opCode));
		(this->*func)(opIndex, adr);

		if(m_callbacks)
		{
			if(m_showSteps)
			{
				m_callbacks->DumpState();
			}
			if(PC == BRK)
			{
				m_callbacks->OnBreak();
			}
			if(PC == opIndex - 1 && m_enableTraps)
			{
				// jump to self: trap
				m_callbacks->OnTrap();
			}
		}
		Cycles(get<3>(opCode));
	}
	else
	{
		throw runtime_error("Unsupported OPcode");
	}
}

} // namespace atre
