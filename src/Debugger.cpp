#include <iostream>
#include <bitset>
#include "Debugger.hpp"

namespace atre
{
void Debugger::DumpCPU(const CPU &cpu)
{
	std::cout << "A = " << std::hex << std::showbase << (int)cpu.A << " (" << std::dec << (int)cpu.A << "), ";
	std::cout << "X = " << std::hex << std::showbase << (int)cpu.X << " (" << std::dec << (int)cpu.X << "), ";
	std::cout << "Y = " << std::hex << std::showbase << (int)cpu.Y << " (" << std::dec << (int)cpu.Y << "), ";
	std::cout << "PC = " << std::hex << std::showbase << (int)cpu.PC << ", ";
	std::cout << "S = " << std::hex << std::showbase << (int)cpu.S << ", ";
	std::cout << "Flags = ";
	std::cout << (cpu.IsSetFlag(CPU::NEGATIVE_FLAG) ? "N" : "n");
	std::cout << (cpu.IsSetFlag(CPU::OVERFLOW_FLAG) ? "O" : "o");
	std::cout << (cpu.IsSetFlag(CPU::IGNORED_FLAG) ? "X" : "x");
	std::cout << (cpu.IsSetFlag(CPU::BREAK_FLAG) ? "B" : "b");
	std::cout << (cpu.IsSetFlag(CPU::DECIMAL_FLAG) ? "D" : "d");
	std::cout << (cpu.IsSetFlag(CPU::INTERRUPT_FLAG) ? "I" : "i");
	std::cout << (cpu.IsSetFlag(CPU::ZERO_FLAG) ? "Z" : "z");
	std::cout << (cpu.IsSetFlag(CPU::CARRY_FLAG) ? "C" : "c");
	std::cout << " " << std::bitset<8>(cpu.F) << std::endl;
}
} // namespace atre
