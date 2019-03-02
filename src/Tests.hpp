#include "atre.hpp"
#include "Atari.hpp"

namespace atre
{
class Tests
{
  public:
	static void CPUFlagsTest(CPU &cpu);
	static void ADC_iTest(CPU &cpu);
	static void OPTest(CPU &cpu);
	static void AddrTest(CPU &cpu);

	static void FunctionalTests(CPU &cpu);

	static void Assert(bool mustBeTrue);
};
} // namespace atre
