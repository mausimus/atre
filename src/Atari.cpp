#include "Atari.hpp"

using namespace std;

namespace atre
{
Atari::Atari()
{
	m_RAM = make_unique<atre::RAM>();
	m_CPU = make_unique<atre::CPU>(getRAM());
	m_IO  = make_unique<atre::IO>(getCPU(), getRAM());
	m_RAM->Connect(getIO());
	m_CPU->Connect(getIO());
}

void Atari::Reset()
{
	m_RAM->Clear();
	m_IO->Reset();
}

void Atari::Boot(const string& osROM, const string& carridgeROM)
{
	Reset();
	m_RAM->LoadROM(osROM, carridgeROM);
	m_CPU->m_enableTraps = true;
	m_CPU->m_showCycles	 = true;
	m_CPU->Reset();
	m_CPU->BreakAt(0xFFFF);
}

} // namespace atre
