#include "Tests.hpp"
#include "Debugger.hpp"

using namespace std;

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

void Tests::FunctionalTest(Atari &atari)
{
	cout << "Loading FunctionalTest" << flush;

	atari.Reset();

	atari.mMemory->Load("6502_functional_test.bin", 0);
	atari.mCPU->mEnableTraps = true;
	atari.mCPU->mShowCycles = true;
	atari.mCPU->JumpTo(0x0400);
	atari.mCPU->BreakAt(0x3469);
}

void Tests::InterruptReg(CPU *cpu, byte_t val)
{
	cpu->_irqPending = val & 1;
	cpu->_nmiPending = val & 2;
}

void Tests::InterruptTest(Atari &atari)
{
	cout << "Loading InterruptTest" << flush;

	atari.Reset();

	std::function<void(byte_t)> interruptFunc = std::bind(&Tests::InterruptReg, atari.mCPU.get(), std::placeholders::_1);

	auto interruptRegister = make_shared<FeedbackRegister>(interruptFunc);
	atari.mMemory->MapFeedbackRegister(0xBFFC, interruptRegister);

	atari.mMemory->Load("6502_interrupt_test.bin", 0xa);
	atari.mCPU->mEnableTraps = true;
	atari.mCPU->JumpTo(0x0400);
	atari.mMemory->Set(0xBFFC, 0);
	atari.mCPU->BreakAt(0x06F5);
}

void Tests::AllSuiteA(Atari &atari)
{
	cout << "Loading AllSuiteA" << flush;

	atari.Reset();

	atari.mMemory->Load("AllSuiteA.bin", 0x4000);
	atari.mCPU->mEnableTraps = true;
	atari.mCPU->mShowCycles = true;
	atari.mCPU->JumpTo(0x4000);
	atari.mCPU->BreakAt(0x45C0);

	//Assert(atari.mMemory->Get(0x0210) == 0xFF);
}

void Tests::TimingTest(Atari &atari)
{
	cout << "Loading TimingTest" << flush;

	atari.Reset();

	atari.mMemory->Load("timingtest-1.bin", 0x1000);
	atari.mCPU->mEnableTraps = true;
	atari.mCPU->mShowCycles = true;
	atari.mCPU->JumpTo(0x1000);
	atari.mCPU->BreakAt(0x1269);

	//	Assert(atari.mCPU->Cycles() == 1141);
}

void Tests::EhBASIC(Atari &atari, shared_ptr<IOPort> keyboardInput, shared_ptr<IOPort> screenOutput)
{
	cout << "Loading EhBasic" << endl;

	atari.Reset();

	//	auto keyboardInput = make_shared<IOPort>();
	atari.mCPU->mMemory->MapIOPort(0xF004, keyboardInput);

	//	auto screenOutput = make_shared<IOPort>();
	atari.mCPU->mMemory->MapIOPort(0xF001, screenOutput);

	atari.mCPU->mMemory->Load("ehbasic.bin", 0xC000);
	atari.mCPU->JumpTo(0xFF80);
	atari.mCPU->mEnableTraps = false;
	atari.mCPU->mShowCycles = false;
}

void Tests::Boot(Atari &atari)
{
	atari.Reset();
	atari.mCPU->mMemory->LoadROM("REV02.ROM", "REVC.ROM");
	atari.mCPU->mEnableTraps = true;
	atari.mCPU->mShowCycles = true;
	atari.mCPU->Reset();
	atari.mCPU->BreakAt(0xA000);
	//atari.mCPU->BreakAt(0xEAA0);
	//atari.mCPU->BreakAt(0xC4F0);
}

void Tests::SelfTest(Atari &atari)
{
	atari.mMemory->DirectSet(ChipRegisters::PORTB, 0b00000001);
	atari.mCPU->JumpTo(0x5000);
}

} // namespace atre
