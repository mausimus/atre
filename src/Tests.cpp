#include "ANTIC.hpp"
#include "Chips.hpp"
#include "Debugger.hpp"
#include "Tests.hpp"

using namespace std;

namespace atre
{
void Tests::Assert(bool mustBeTrue)
{
	if(!mustBeTrue)
	{
		throw std::runtime_error("Assertion failed");
	}
	else
	{
		std::cout << "Assertion passed" << std::endl;
	}
}

void Tests::FunctionalTest(Atari& atari)
{
	cout << "Loading FunctionalTest" << flush;

	atari.Reset();

	atari.RAM()->Load("6502_functional_test.bin", 0);
	atari.CPU()->m_enableTraps = true;
	atari.CPU()->m_showCycles  = true;
	atari.CPU()->JumpTo(0x0400);
	atari.CPU()->BreakAt(0x3469);
}

void Tests::InterruptReg(CPU* cpu, byte_t val)
{
	cpu->m_irqPending = val & 1;
	cpu->m_nmiPending = val & 2;
}

void Tests::InterruptTest(Atari& atari)
{
	cout << "Loading InterruptTest" << flush;

	atari.Reset();

	std::function<void(byte_t)> interruptFunc =
		std::bind(&Tests::InterruptReg, atari.CPU(), std::placeholders::_1);

	auto interruptRegister = make_shared<FeedbackRegister>(interruptFunc);
	atari.RAM()->MapFeedbackRegister(0xBFFC, interruptRegister);

	atari.RAM()->Load("6502_interrupt_test.bin", 0xa);
	atari.CPU()->m_enableTraps = true;
	atari.CPU()->JumpTo(0x0400);
	atari.RAM()->Set(0xBFFC, 0);
	atari.CPU()->BreakAt(0x06F5);
}

void Tests::AllSuiteA(Atari& atari)
{
	cout << "Loading AllSuiteA" << flush;

	atari.Reset();

	atari.RAM()->Load("AllSuiteA.bin", 0x4000);
	atari.CPU()->m_enableTraps = true;
	atari.CPU()->m_showCycles  = true;
	atari.CPU()->JumpTo(0x4000);
	atari.CPU()->BreakAt(0x45C0);

	// Assert(atari.RAM()->Get(0x0210) == 0xFF);
}

void Tests::TimingTest(Atari& atari)
{
	cout << "Loading TimingTest" << flush;

	atari.Reset();

	atari.RAM()->Load("timingtest-1.bin", 0x1000);
	atari.CPU()->m_enableTraps = true;
	atari.CPU()->m_showCycles  = true;
	atari.CPU()->JumpTo(0x1000);
	atari.CPU()->BreakAt(0x1269);

	//	Assert(atari.CPU()->Cycles() == 1141);
}

void Tests::Boot(Atari& atari, const std::string& osROM, const std::string& carridgeROM)
{
	atari.Reset();
	//	atari.CPU()->RAM()->LoadROM("REV02.ROM", "REVC.ROM");
	atari.CPU()->m_RAM->LoadROM(osROM, carridgeROM);
	atari.CPU()->m_enableTraps = true;
	atari.CPU()->m_showCycles  = true;
	atari.CPU()->Reset();
	atari.CPU()->BreakAt(0xFFFF);
	// atari.CPU()->BreakAt(0xA000); // BASIC
}

void Tests::SelfTest(Atari& atari)
{
	atari.RAM()->DirectSet(ChipRegisters::PORTB, 0b00000001);
	atari.CPU()->JumpTo(0x5000);
}
} // namespace atre
