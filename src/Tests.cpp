#include "ANTIC.hpp"
#include "Chips.hpp"
#include "Debugger.hpp"
#include "Tests.hpp"

using namespace std;

namespace atre
{

TestCallbacks::TestCallbacks() : Callbacks(), m_trapHit() {}

void TestCallbacks::OnTrap()
{
	m_trapHit = true;
}

void TestCallbacks::OnBreak()
{
	m_trapHit = true;
}

bool TestCallbacks::IsTrap() const
{
	return m_trapHit;
}

void Tests::Assert(bool mustBeTrue)
{
	if(!mustBeTrue)
	{
		throw runtime_error("Assertion failed");
	}
	else
	{
		cout << "Assertion passed" << endl;
	}
}

void Tests::FunctionalTest(const string& romFile)
{
	if(!filesystem::exists(romFile))
	{
		cout << "FunctionalTest ROM " << romFile << " not found" << endl;
		return;
	}

	cout << "FunctionalTest: " << flush;

	TestCallbacks tc;
	RAM			  ram;
	CPU			  cpu(&ram);

	ram.Load(romFile, 0);
	cpu.Attach(&tc);
	cpu.m_enableTraps = true;
	cpu.m_showCycles  = true;
	cpu.JumpTo(0x0400);
	cpu.BreakAt(0x3469);
	while(!tc.IsTrap())
	{
		cpu.Execute();
	}
	Assert(cpu.PC == 0x3469);
}

void Tests::InterruptReg(CPU* cpu, byte_t val)
{
	cpu->m_irqPending = val & 1;
	cpu->m_nmiPending = val & 2;
}

void Tests::InterruptTest(const string& romFile)
{
	if(!filesystem::exists(romFile))
	{
		cout << "InterruptTest ROM " << romFile << " not found" << endl;
		return;
	}

	cout << "InterruptTest: " << flush;

	TestCallbacks tc;
	RAM			  ram;
	CPU			  cpu(&ram);

	function<void(byte_t)> interruptFunc = bind(&Tests::InterruptReg, &cpu, placeholders::_1);

	auto interruptRegister = make_shared<FeedbackRegister>(interruptFunc);
	ram.MapFeedbackRegister(0xBFFC, interruptRegister);

	ram.Load(romFile, 0xa);
	ram.Set(0xBFFC, 0);
	cpu.Attach(&tc);
	cpu.m_enableTraps = true;
	cpu.m_showCycles  = true;
	cpu.JumpTo(0x0400);
	cpu.BreakAt(0x06F5);
	while(!tc.IsTrap())
	{
		cpu.Execute();
	}
	Assert(cpu.PC == 0x06F5);
}

void Tests::AllSuiteA(const string& romFile)
{
	if(!filesystem::exists(romFile))
	{
		cout << "AllSuiteA ROM " << romFile << " not found" << endl;
		return;
	}

	cout << "AllSuiteA: " << flush;

	TestCallbacks tc;
	RAM			  ram;
	CPU			  cpu(&ram);

	ram.Load(romFile, 0x4000);
	cpu.Attach(&tc);
	cpu.m_enableTraps = true;
	cpu.m_showCycles  = true;
	cpu.JumpTo(0x4000);
	cpu.BreakAt(0x45C0);
	while(!tc.IsTrap())
	{
		cpu.Execute();
	}
	Assert(ram.Get(0x0210) == 0xFF);
}

void Tests::TimingTest(const string& romFile)
{
	if(!filesystem::exists(romFile))
	{
		cout << "TimingTest ROM " << romFile << " not found" << endl;
		return;
	}

	cout << "TimingTest: " << flush;

	TestCallbacks tc;
	RAM			  ram;
	CPU			  cpu(&ram);

	ram.Load(romFile, 0x1000);
	cpu.Attach(&tc);
	cpu.m_enableTraps = true;
	cpu.m_showCycles  = true;
	cpu.JumpTo(0x1000);
	cpu.BreakAt(0x1269);
	while(!tc.IsTrap())
	{
		cpu.Execute();
	}
	Assert(cpu.Cycles() == 1141);
}
} // namespace atre
