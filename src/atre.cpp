#include <iostream>
#include "Atari.hpp"
#include "Tests.hpp"
#include "Debugger.hpp"

using namespace atre;

int main(int /*argc*/, char * /*argv*/ [])
{
	Atari atari;

	//	Tests::CPUFlagsTest(atari.mCPU);
	//	Debugger::DumpCPU(atari.mCPU);
	//Tests::ADC_iTest(atari.mCPU);
	//	Tests::OPTest(atari.mCPU);
	//	Tests::AddrTest(atari.mCPU);

	atari.mMemory.Load("6502ft.bin", 0);
	atari.mCPU.EntryPoint(0x400);
	do
	{
		Debugger::DumpCPU(atari.mCPU);
		atari.mCPU.Execute();
	} while (atari.mCPU.Cycles() < 1000000);

	std::cout << "Done!" << std::endl;
}
