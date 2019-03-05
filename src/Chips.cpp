#include "Chips.hpp"

using namespace std;

namespace atre
{
ChipIO::ChipIO(CPU *cpu, Memory *memory) : _GTIA(cpu, memory),
										   _ANTIC(cpu, memory),
										   _POKEY(cpu, memory),
										   _PIA(cpu, memory)
{
}

void ChipIO::Write(word_t addr, byte_t val)
{
	if (addr < 0xD000 || addr >= 0xD800)
	{
		throw runtime_error("Invalid IO address");
	}
	if (addr < 0xD200)
	{
		_GTIA.Write(addr & 0x1F, val);
	}
	if (addr < 0xD300)
	{
		_POKEY.Write(addr & 0xF, val);
	}
	if (addr < 0xD400)
	{
		_PIA.Write(addr & 0x3, val);
	}
	_ANTIC.Write(addr & 0xF, val);
}

byte_t ChipIO::Read(word_t addr)
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
	return _ANTIC.Read(0xD400 + (addr & 0xF));
}

void ChipIO::Tick()
{
	_GTIA.Tick();
	_POKEY.Tick();
	_PIA.Tick();
	_ANTIC.Tick();
}

void ChipIO::Reset()
{
	_GTIA.Reset();
	_POKEY.Reset();
	_PIA.Reset();
	_ANTIC.Reset();
}

byte_t GTIA::Read(word_t addr)
{
	switch (addr)
	{
	case ChipRegisters::PAL:
		return 0xF;
	case ChipRegisters::CONSOL:
		return 0x7;
	default:
		break;
	}

	return mMemory->DirectGet(addr);
}

void GTIA::Write(word_t addr, byte_t val)
{
	switch (addr)
	{
	case ChipRegisters::COLBK:
	case ChipRegisters::PRIOR:
	case ChipRegisters::VDELAY:
	case ChipRegisters::CONSOL:
	default:
		mMemory->DirectSet(addr, val);
		break;
	}
}

byte_t POKEY::Read(word_t addr)
{
	switch (addr)
	{
	case ChipRegisters::KBCODE:
		return 0;
	case ChipRegisters::IRQST:
		return 0;
	default:
		break;
	}

	return mMemory->DirectGet(addr);
}

void POKEY::Write(word_t addr, byte_t val)
{
	switch (addr)
	{
	case ChipRegisters::IRQEN:
	case ChipRegisters::STIMER:
	default:
		mMemory->DirectSet(addr, val);
		break;
	}
}

byte_t PIA::Read(word_t addr)
{
	switch (addr)
	{
	case ChipRegisters::PORTB:
	default:
		break;
	}

	return mMemory->DirectGet(addr);
}

void PIA::Write(word_t addr, byte_t val)
{
	switch (addr)
	{
	case ChipRegisters::PORTB:
	default:
		mMemory->DirectSet(addr, val);
		break;
	}
}

void PIA::Reset()
{
	// enable all ROMs by default
	mMemory->DirectSet(ChipRegisters::PORTB, 0b00000001);
}

byte_t ANTIC::Read(word_t addr)
{
	switch (addr)
	{
	case ChipRegisters::VCOUNT:
	{
		return _lineNum / 2;
	}
	case ChipRegisters::NMIST:
	default:
		break;
	}

	return mMemory->DirectGet(addr);
}

void ANTIC::Write(word_t addr, byte_t val)
{
	switch (addr)
	{
	case ChipRegisters::NMIRES:
		mMemory->DirectSet(ChipRegisters::NMIST, 0);
		return;
	case ChipRegisters::NMIEN:
	default:
		mMemory->DirectSet(addr, val);
		break;
	}
}

void ANTIC::Reset()
{
	_lineNum = 0;
	_lineCycle = 0;
	mMemory->DirectSet(ChipRegisters::NMIEN, 0);
	mMemory->DirectSet(ChipRegisters::NMIST, 0);
}

void ANTIC::Tick()
{
	_lineCycle++;
	if (_lineCycle == CYCLES_PER_SCANLINE)
	{
		_lineCycle = 0;
		_lineNum++;
		if (_lineNum == NUM_SCANLINES)
		{
			_lineNum = 0;
			if (mMemory->DirectGet(ChipRegisters::NMIEN) & 64)
			{
				// VBlank interrupt
				mMemory->DirectSet(ChipRegisters::NMIST, 64);
				mCPU->NMI();
			}
		}
	}
}

} // namespace atre
