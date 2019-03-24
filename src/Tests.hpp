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
		static void Boot(Atari &atari, const std::string& osROM, const std::string& cartridgeROM = "");
		static void SelfTest(Atari &atari);

		static void InterruptReg(CPU *cpu, byte_t val);
		static void Assert(bool mustBeTrue);
	};
} // namespace atre