#include "atre.hpp"
#include "Atari.hpp"

namespace atre
{
class Tests
{
  public:
	static void FunctionalTest(Atari &atari);
	static void AllSuiteA(Atari &atari);
	static void TimingTest(Atari &atari);
	static void EhBASIC(Atari &atari);

	static void Assert(bool mustBeTrue);
};
} // namespace atre
