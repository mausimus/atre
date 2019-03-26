#include "Atari.hpp"
#include "atre.hpp"

namespace atre
{

class TestCallbacks : public Callbacks
{
	bool m_trapHit;

public:
	TestCallbacks();

	void OnTrap() override;
	void OnBreak() override;

	bool IsTrap() const;
};

class Tests
{
	static void InterruptReg(CPU* cpu, byte_t val);
	static void Assert(bool mustBeTrue);

public:
	static void FunctionalTest(const std::string& romFile = "6502_functional_test.bin");
	static void InterruptTest(const std::string& romFile = "6502_interrupt_test.bin");
	static void AllSuiteA(const std::string& romFile = "AllSuiteA.bin");
	static void TimingTest(const std::string& romFile = "timingtest-1.bin");
};
} // namespace atre
