#include "Chips.hpp"
#include "ANTIC.hpp"

using namespace std;

namespace atre
{
ChipIO::ChipIO(CPU *cpu, Memory *memory) : _GTIA(cpu, memory),
										   _ANTIC(),
										   _POKEY(cpu, memory),
										   _PIA(cpu, memory)
{
	_ANTIC = make_unique<ANTIC>(cpu, memory);
}

void ChipIO::Write(word_t addr, byte_t val)
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
	return _ANTIC->Read(0xD400 + (addr & 0xF));
}

void ChipIO::KeyboardInput(const std::string &input)
{
	_POKEY.KeyboardInput(input);
}

void ChipIO::Tick()
{
	_GTIA.Tick();
	_POKEY.Tick();
	_PIA.Tick();
	_ANTIC->Tick();
}

void ChipIO::Reset()
{
	_GTIA.Reset();
	_POKEY.Reset();
	_PIA.Reset();
	_ANTIC->Reset();
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
	case ChipRegisters::CONSOL:
	{
		int x = 4;
		x++;
	}
	break;
	case ChipRegisters::COLBK:
	case ChipRegisters::PRIOR:
	case ChipRegisters::VDELAY:
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
		_irqStatus &= ~(0b01000000);
		return _kbCode;
	case ChipRegisters::SKSTAT:
		return 0;
	case ChipRegisters::IRQST:
		return ~_irqStatus;
	default:
		break;
	}

	return mMemory->DirectGet(addr);
}

void POKEY::Write(word_t addr, byte_t val)
{
	switch (addr)
	{
	case ChipRegisters::SEROUT:
		_sioComplete = false;
		for (int i = 0; i < 20; i++)
		{
			// 20 cycles per byte?
			_serialOut.push_back(val);
		}
		break;
	case ChipRegisters::IRQEN:
		_irqStatus &= val; // clear any pendng interrupts
		if (_sioComplete)
		{
			_irqStatus |= 0b1000; // always set serial status
		}
		mMemory->DirectSet(addr, val);
		break;
	case ChipRegisters::STIMER:
	default:
		mMemory->DirectSet(addr, val);
		break;
	}
}

map<char, byte_t> POKEY::_scanCodes = {
	{'p', 0x0A},
	{'r', 0x28},
	{'i', 0x0D},
	{'n', 0x23},
	{'t', 0x2D},
	{'a', 0x3F},
	{'e', 0x2A},
	{'\r', 0x0C},
	{'\n', 0x0C},
	{'"', (0x1E + 0b01000000)},
	{' ', 0x21},
	{'f', 0x38}};

void POKEY::KeyboardInput(const std::string &input)
{
	_keyBuffer.append(input);
}

void POKEY::Tick()
{
	auto irqen = mMemory->DirectGet(ChipRegisters::IRQEN);
	if (_serialOut.size() > 1)
	{
		_serialOut.pop_back();
	}
	else if (_serialOut.size() == 1)
	{
		_serialOut.clear();
		_sioComplete = true;
		if (irqen & 0b10000)
		{
			_irqStatus |= 0b10000;
			mCPU->IRQ(); // serial output ready
		}
	}
	if (_sioComplete && (irqen & 0b1000))
	{
		mCPU->IRQ(); // serial output complete
	}
	_cycles++;
	if (_cycles == 1000000)
	{
		_cycles = 0;
		if (_keyBuffer.length())
		{
			auto c = _keyBuffer[0];
			_keyBuffer = _keyBuffer.substr(1);
			auto scanCode = _scanCodes.find(c);
			if (scanCode != _scanCodes.end())
			{
				_kbCode = scanCode->second;
				if (irqen & 0b1000000)
				{
					_irqStatus |= 0b1000000;
					mCPU->IRQ(); // keypress
				}
			}
		}
	}
	/* TODO: timers
	_cycles++;
	if (_cycles == 10000)
	{
		_cycles = 0;
		bool trigger = false;
		if (irqen & 1)
		{
			trigger = true;
			_irqStatus |= 1;
		}
		if (irqen & 2)
		{
			trigger = true;
			_irqStatus |= 2;
		}
		if (irqen & 4)
		{
			trigger = true;
			_irqStatus |= 4;
		}
		if (trigger)
		{
			mCPU->IRQ();
		}
	}*/
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

} // namespace atre
