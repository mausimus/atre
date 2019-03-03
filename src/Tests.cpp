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
	cout << "Running FunctionalTest: " << flush;

	atari.mCPU->Reset();

	atari.mMemory->Load("6502_functional_test.bin", 0);
	atari.mCPU->mEnableTraps = true;
	atari.mCPU->mShowCycles = true;
	atari.mCPU->JumpTo(0x0400);
	atari.mCPU->ExecuteUntil(0x3469);

	Assert(true);
}

void Tests::AllSuiteA(Atari &atari)
{
	cout << "Running AllSuiteA: " << flush;

	atari.mCPU->Reset();

	atari.mMemory->Load("AllSuiteA.bin", 0x4000);
	atari.mCPU->mEnableTraps = true;
	atari.mCPU->mShowCycles = true;
	atari.mCPU->JumpTo(0x4000);
	atari.mCPU->ExecuteUntil(0x45C0);

	Assert(atari.mMemory->Get(0x0210) == 0xFF);
}

void Tests::TimingTest(Atari &atari)
{
	cout << "Running TimingTest: " << flush;

	atari.mCPU->Reset();

	atari.mMemory->Load("timingtest-1.bin", 0x1000);
	atari.mCPU->mEnableTraps = true;
	atari.mCPU->mShowCycles = true;
	atari.mCPU->JumpTo(0x1000);
	atari.mCPU->ExecuteUntil(0x1269);

	Assert(atari.mCPU->Cycles() == 1141);
}

void Tests::EhBASIC(Atari &atari)
{
	cout << "Starting EhBasic" << endl;

	atari.mCPU->Reset();

	IOPort keyboardInput;
	atari.mCPU->mMemory->MapIOPort(0xF004, &keyboardInput);
	const string command{"C32768\r\n10 FOR I = 1 TO 10\r\n20 PRINT I\r\n30 NEXT I\r\nRUN\r\n"};
	for (size_t i = 0; i < command.length(); i++)
	{
		keyboardInput.queue.push_back(command[i]);
	}

	IOPort screenOutput;
	atari.mCPU->mMemory->MapIOPort(0xF001, &screenOutput);

	atari.mCPU->mMemory->Load("ehbasic.bin", 0xC000);
	atari.mCPU->JumpTo(0xFF80);
	atari.mCPU->mEnableTraps = false;
	atari.mCPU->mShowCycles = false;

	for (;;)
	{
		atari.mCPU->Execute();
		while (!screenOutput.queue.empty())
		{
			cout << screenOutput.queue.front() << flush;
			screenOutput.queue.pop_front();
		}
	};
}

} // namespace atre
