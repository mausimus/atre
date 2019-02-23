#include "Tests.hpp"
#include "Debugger.hpp"

namespace atre
{

void Tests::Assert(bool mustBeTrue)
{
	if (!mustBeTrue)
	{
		throw std::runtime_error("Assertion failed");
	}
	else
	{
		std::cout << "Assertion passed" << std::endl;
	}
}

void Tests::CPUFlagsTest(CPU &cpu)
{
	cpu.SetFlag(CPU::NEGATIVE_FLAG);
	std::cout << cpu.IsSetFlag(CPU::CARRY_FLAG) << std::endl;
	cpu.SetFlag(CPU::CARRY_FLAG);
	std::cout << cpu.IsSetFlag(CPU::CARRY_FLAG) << std::endl;
	cpu.ClearFlag(CPU::CARRY_FLAG);
	std::cout << cpu.IsSetFlag(CPU::CARRY_FLAG) << std::endl;
	std::cout << cpu.IsSetFlag(CPU::NEGATIVE_FLAG) << std::endl;
}

void Tests::ADC_iTest(CPU &cpu)
{
	std::cout << "ADC_iTest" << std::endl;

	cpu.A = 2;
	cpu.F = 0;
	cpu.ADC(3);
	Debugger::DumpCPU(cpu);

	cpu.A = 2;
	cpu.F = CPU::CARRY_FLAG;
	cpu.ADC(3);
	Debugger::DumpCPU(cpu);

	cpu.A = 2;
	cpu.F = 0;
	cpu.ADC(254);
	Debugger::DumpCPU(cpu);

	cpu.A = 2;
	cpu.F = 0;
	cpu.ADC(253);
	Debugger::DumpCPU(cpu);

	cpu.A = 253;
	cpu.F = CPU::NEGATIVE_FLAG;
	cpu.ADC(6);
	Debugger::DumpCPU(cpu);

	cpu.A = 125;
	cpu.F = CPU::CARRY_FLAG;
	cpu.ADC(2);
	Debugger::DumpCPU(cpu);
}
void Tests::OPTest(CPU &cpu)
{
	cpu.mMemory.Set(0, 0x69);
	cpu.mMemory.Set(1, 7);
	cpu.A = 1;
	cpu.SetFlag(CPU::CARRY_FLAG);
	cpu.Execute();
	Debugger::DumpCPU(cpu);
}
void Tests::AddrTest(CPU &cpu)
{
	cpu.Reset();
	cpu.mMemory.Set(0, 1);
	Assert(cpu.GetOP(0, Addressing::Immediate) == 1);

	cpu.mMemory.SetW(0, 0x2123);
	cpu.mMemory.Set(0x2123, 2);
	Assert(cpu.GetOP(0, Addressing::Absolute) == 2);

	cpu.mMemory.Set(0, 0x30);
	cpu.mMemory.Set(0x30, 3);
	Assert(cpu.GetOP(0, Addressing::ZeroPage) == 3);

	cpu.X = 2;
	cpu.mMemory.Set(0, 0x40);
	cpu.mMemory.Set(0x42, 4);
	Assert(cpu.GetOP(0, Addressing::ZeroPageX) == 4);

	// wraparound
	cpu.X = 0xFF;
	cpu.mMemory.Set(0, 0x80);
	cpu.mMemory.Set(0x7F, 5);
	Assert(cpu.GetOP(0, Addressing::ZeroPageX) == 5);

	cpu.X = 0x10;
	cpu.mMemory.SetW(0, 0x60F0);
	cpu.mMemory.Set(0x6100, 6);
	Assert(cpu.GetOP(0, Addressing::AbsoluteX) == 6);

	cpu.Y = 0x11;
	cpu.mMemory.SetW(0, 0x60F1);
	cpu.mMemory.Set(0x6102, 7);
	Assert(cpu.GetOP(0, Addressing::AbsoluteY) == 7);

	cpu.Y = 6;
	cpu.mMemory.Set(0, 0x43);
	cpu.mMemory.Set(0x43, 0x53);
	cpu.mMemory.Set(0x44, 0xE4);
	Assert(cpu.mMemory.GetW(0x43) == 0xE453);
	cpu.mMemory.Set(0xE459, 8);
	Assert(cpu.GetOP(0, Addressing::IndirectIndexed) == 8);

	cpu.X = 4;
	cpu.mMemory.Set(0, 0x43);
	cpu.mMemory.SetW(0x47, 0xE453);
	cpu.mMemory.Set(0xE453, 9);
	Assert(cpu.GetOP(0, Addressing::IndexedIndirect) == 9);
}
} // namespace atre
