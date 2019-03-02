#include <iostream>
#include "Atari.hpp"
#include "Tests.hpp"
#include "Debugger.hpp"

using namespace atre;
using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{
	Atari atari;

	Tests::FunctionalTest(atari);
	Tests::AllSuiteA(atari);
	Tests::EhBASIC(atari);

	std::cout << "Done!" << std::endl;
}
