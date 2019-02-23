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

	Tests::AddrTest(atari.mCPU);

	std::cout << "Done!" << std::endl;
}
