#include "IO.hpp"
#include "ANTIC.hpp"

using namespace std;

namespace atre
{
IO::IO(CPU *cpu, Memory *memory) : _GTIA(cpu, memory),
								   _ANTIC(),
								   _POKEY(cpu, memory),
								   _PIA(cpu, memory)
{
	_ANTIC = make_unique<ANTIC>(cpu, memory);
}

void IO::Write(word_t addr, byte_t val)
{
	if (addr < 0xD000 || addr >= 0xD800)
	{
		throw runtime_error("Invalid IO address");
	}
	if (addr < 0xD200)
	{
		_GTIA.Write(addr /*0xD000 + (addr & 0x1F)*/, val);
		return;
	}
	if (addr < 0xD300)
	{
		_POKEY.Write(addr /*0xD200 + (addr & 0xF)*/, val);
		return;
	}
	if (addr < 0xD400)
	{
		_PIA.Write(addr /*0xD300 + (addr & 0x3)*/, val);
		return;
	}
	_ANTIC->Write(addr /*0xD400 + (addr & 0xF)*/, val);
}

byte_t IO::Read(word_t addr)
{
	if (addr < 0xD000 || addr >= 0xD800)
	{
		throw runtime_error("Invalid IO address");
	}
	if (addr < 0xD200)
	{
		return _GTIA.Read(0xD000 + (addr & 0x1F));
	}
	if (addr < 0xD300)
	{
		return _POKEY.Read(0xD200 + (addr & 0xF));
	}
	if (addr < 0xD400)
	{
		return _PIA.Read(0xD300 + (addr & 0x3));
	}
	return _ANTIC->Read(0xD400 + (addr & 0xF));
}

void IO::KeyboardInput(const std::string &input)
{
	_POKEY.KeyboardInput(input);
}

void IO::Tick()
{
	_GTIA.Tick();
	_POKEY.Tick();
	_PIA.Tick();
	_ANTIC->Tick();
}

void IO::Reset()
{
	_GTIA.Reset();
	_POKEY.Reset();
	_PIA.Reset();
	_ANTIC->Reset();
}

} // namespace atre
