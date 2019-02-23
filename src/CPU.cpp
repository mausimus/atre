#include "CPU.hpp"

namespace atre
{
CPU::CPU(Memory &memory) : mMemory(memory)
{
	InitializeOPCodes();
	Reset();
}

void CPU::Reset()
{
	A = X = Y = S = PC = F = 0;
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

void CPU::Cycles(unsigned long cycles)
{
	_cycles += cycles;
}

byte_t CPU::GetOP(word_t opIndex, Addressing adr)
{
	switch (adr)
	{
	case Addressing::Immediate:
	{
		return mMemory.Get(opIndex);
	}
	case Addressing::Absolute:
	{
		word_t addr = mMemory.GetW(opIndex);
		return mMemory.Get(addr);
	}
	case Addressing::ZeroPage:
	{
		return mMemory.Get(mMemory.Get(opIndex));
	}
	case Addressing::ZeroPageX:
	{
		return mMemory.Get((mMemory.Get(opIndex) + X) & 0xFF);
	}
	case Addressing::AbsoluteX:
	{
		word_t baseAddr = mMemory.GetW(opIndex);
		auto finalAddr = baseAddr + X;
		if (baseAddr >> 8 != finalAddr >> 8)
		{
			Cycles(1);
		}
		return mMemory.Get(finalAddr);
	}
	case Addressing::AbsoluteY:
	{
		word_t baseAddr = mMemory.GetW(opIndex);
		auto finalAddr = baseAddr + Y;
		if (baseAddr >> 8 != finalAddr >> 8)
		{
			Cycles(1);
		}
		return mMemory.Get(finalAddr);
	}
	case Addressing::IndexedIndirect:
	{
		word_t addr = mMemory.GetW(mMemory.Get(opIndex) + X);
		return mMemory.Get(addr);
	}
	case Addressing::IndirectIndexed:
	{
		byte_t basePointer = mMemory.Get(opIndex);
		word_t baseAddress = mMemory.GetW(basePointer);
		word_t finalAddress = baseAddress + Y;
		if (baseAddress >> 8 != finalAddress >> 8)
		{
			Cycles(1);
		}
		return mMemory.Get(finalAddress);
	}
	default:
		throw std::runtime_error("Not supported addressing mode");
	}
}

void CPU::ADC(byte_t op)
{
	if (IsSetFlag(CPU::DECIMAL_FLAG))
	{
		throw std::runtime_error("Not supported");
	}
	else
	{
		word_t result = A;
		result += op;
		if (IsSetFlag(CPU::CARRY_FLAG))
		{
			result++;
		}
		byte_t res = result & 0xFF;
		SetFlag(CPU::CARRY_FLAG, result > 255);
		SetFlag(CPU::NEGATIVE_FLAG, IsNegative(res));
		SetFlag(CPU::ZERO_FLAG, IsZero(res));
		SetFlag(CPU::OVERFLOW_FLAG,
				IsNegative(A) != IsNegative(res));
		A = res;
	}
}

void CPU::opADC(word_t opIndex, Addressing adr)
{
	ADC(GetOP(opIndex, adr));
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
	_opCodeMap[0x7D] = std::make_tuple(&atre::CPU::opADC, Addressing::AbsoluteX, 3, 4);
	_opCodeMap[0x79] = std::make_tuple(&atre::CPU::opADC, Addressing::AbsoluteY, 3, 4);
	_opCodeMap[0x61] = std::make_tuple(&atre::CPU::opADC, Addressing::IndexedIndirect, 2, 6);
	_opCodeMap[0x71] = std::make_tuple(&atre::CPU::opADC, Addressing::IndirectIndexed, 2, 5);
}

void CPU::Execute()
{
	byte_t code = mMemory.Get(PC);
	auto &opCode = _opCodeMap[code];
	if (std::get<2>(opCode) > 0)
	{
		Cycles(std::get<3>(opCode));
		auto adr = std::get<1>(opCode);
		auto func = std::get<0>(opCode);
		(this->*func)(PC + 1, adr);
		PC += std::get<2>(opCode); // TO DO: revisit when implementing jumps
	}
	else
	{
		throw std::runtime_error("Unsupported OPcode");
	}
}

} // namespace atre
