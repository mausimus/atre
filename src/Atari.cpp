#include "Atari.hpp"

using namespace std;

namespace atre
{
Atari::Atari()
{
	m_RAM = make_unique<atre::RAM>();
	m_CPU = make_unique<atre::CPU>(RAM());
	m_IO  = make_unique<atre::IO>(CPU(), RAM());
	m_RAM->Connect(IO());
	m_CPU->Connect(IO());
}

CPU* Atari::CPU()
{
	return m_CPU.get();
}

IO* Atari::IO()
{
	return m_IO.get();
}

RAM* Atari::RAM()
{
	return m_RAM.get();
}

void Atari::Reset()
{
	m_RAM->Clear();
	m_IO->Reset();
}
} // namespace atre
