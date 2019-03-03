#include "atre.hpp"
#include "Atari.hpp"

namespace atre
{
class Tests
{
  public:
	static void FunctionalTest(Atari &atari);
	static void InterruptTest(Atari &atari);
	static void AllSuiteA(Atari &atari);
	static void TimingTest(Atari &atari);
	static void EhBASIC(Atari &atari, std::shared_ptr<IOPort> keyboardInput, std::shared_ptr<IOPort> screenOutput);

	static void InterruptReg(CPU *cpu, byte_t val);
	static void Assert(bool mustBeTrue);
};
} // namespace atre
