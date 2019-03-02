#include <iostream>
#include "Atari.hpp"
#include "Tests.hpp"
#include "Debugger.hpp"

using namespace atre;

int main(int /*argc*/, char * /*argv*/ [])
{
	Atari atari;

	atari.mMemory.Load("AllSuiteA.bin", 0x4000);
	//atari.mCPU.EntryPoint(0x400, 0x3469);
	atari.mCPU.EntryPoint(0x4000, 0x45C0);
	do
	{
		Debugger::DumpCPU(atari.mCPU);
		atari.mCPU.Execute();
	} while (atari.mCPU.Cycles() < 100000000);

	std::cout << "Done!" << std::endl;
}
