#include "ANTIC.hpp"
#include "Chips.hpp"

using namespace std;

namespace atre
{
byte_t GTIA::Read(word_t addr)
{
	switch(addr)
	{
	case ChipRegisters::M0PF:
	case ChipRegisters::M1PF:
	case ChipRegisters::M2PF:
	case ChipRegisters::M3PF:
	case ChipRegisters::P0PF:
	case ChipRegisters::P1PF:
	case ChipRegisters::P2PF:
	case ChipRegisters::P3PF:
	case ChipRegisters::M0PL:
	case ChipRegisters::M1PL:
	case ChipRegisters::M2PL:
	case ChipRegisters::M3PL:
	case ChipRegisters::P0PL:
	case ChipRegisters::P1PL:
	case ChipRegisters::P2PL:
	case ChipRegisters::P3PL:
		return m_collisions[addr & 0xF];
	case ChipRegisters::PAL:
		return 0xF;
	case ChipRegisters::TRIG0:
		return !m_joyFire;
	case ChipRegisters::TRIG1:
		return 0x1;
	case ChipRegisters::CONSOL:
	{
		byte_t consol = 0;
		if(!m_optionKey)
		{
			consol += 1 << 2;
		}
		if(!m_selectKey)
		{
			consol += 1 << 1;
		}
		if(!m_startKey)
		{
			consol += 1;
		}
		return consol;
	}
	default:
		break;
	}

	return m_RAM->DirectGet(addr);
}

void GTIA::Write(word_t addr, byte_t val)
{
	switch(addr)
	{
	case ChipRegisters::HITCLR:
		memset(m_collisions, 0, sizeof(m_collisions));
		break;
	case ChipRegisters::CONSOL:
		break;
	case ChipRegisters::COLBK:
	case ChipRegisters::PRIOR:
	case ChipRegisters::VDELAY:
	default:
		m_RAM->DirectSet(addr, val);
		break;
	}
}

byte_t POKEY::Read(word_t addr)
{
	switch(addr)
	{
	case ChipRegisters::KBCODE:
		m_irqStatus &= ~(0b01000000);
		return m_scanCode;
	case ChipRegisters::SKSTAT:
		return ~m_kbStat;
	case ChipRegisters::IRQST:
		return ~m_irqStatus;
	default:
		break;
	}

	return m_RAM->DirectGet(addr);
}

void POKEY::Write(word_t addr, byte_t val)
{
	switch(addr)
	{
	case ChipRegisters::SEROUT:
		m_sioComplete = false;
		for(int i = 0; i < 20; i++)
		{
			m_serialOut.push_back(val);
		}
		break;
	case ChipRegisters::IRQEN:
		m_irqStatus &= val; // clear any pendng interrupts
		if(m_sioComplete)
		{
			m_irqStatus |= 0b1000; // always set serial status
		}
		m_RAM->DirectSet(addr, val);
		break;
	case ChipRegisters::STIMER:
	default:
		m_RAM->DirectSet(addr, val);
		break;
	}
}

POKEY::POKEY(CPU* cpu, RAM* ram) :
	Chip(cpu, ram), m_serialOut(), m_irqStatus(0), m_scanCode(), m_kbStat(), m_sioComplete(),
	m_keyPressed(), m_breakPressed(), m_cycles()
{}

void POKEY::KeyDown(byte_t scanCode, bool shiftStatus, bool ctrlStatus)
{
	m_scanCode = scanCode;

	if(shiftStatus)
	{
		m_scanCode |= 0b1000000;
		m_kbStat |= 0b1000;
	}
	else
	{
		m_kbStat &= ~(0b1000);
	}

	if(ctrlStatus)
	{
		m_scanCode |= 0b10000000;
	}

	m_kbStat |= 0b100;
	m_keyPressed = true;
}

void POKEY::KeyUp()
{
	m_kbStat &= ~(0b100);
}

void POKEY::Break()
{
	m_breakPressed = true;
}

void POKEY::ShiftKey(bool shiftStatus)
{
	if(shiftStatus)
	{
		m_kbStat |= 0b1000;
	}
	else
	{
		m_kbStat &= ~(0b1000);
	}
}

void POKEY::Tick()
{
	auto irqen = m_RAM->DirectGet(ChipRegisters::IRQEN);
	if(m_serialOut.size() > 1)
	{
		m_serialOut.pop_back();
	}
	else if(m_serialOut.size() == 1)
	{
		m_serialOut.clear();
		m_sioComplete = true;
		if(irqen & 0b10000)
		{
			m_irqStatus |= 0b10000;
			m_CPU->IRQ(); // serial output ready
		}
	}
	if(m_sioComplete && (irqen & 0b1000))
	{
		m_CPU->IRQ(); // serial output complete
	}
	if(m_keyPressed)
	{
		if((irqen & 0b1000000))
		{
			m_irqStatus |= 0b1000000;
			m_CPU->IRQ(); // keypress
		}
		m_keyPressed = false;
	}
	if(m_breakPressed)
	{
		if((irqen & 0b10000000))
		{
			m_irqStatus |= 0b10000000;
			m_CPU->IRQ(); // break
		}
		m_breakPressed = false;
	}

	m_cycles++;
	if(m_cycles == CYCLES_PER_SEC)
	{
		m_cycles = 0;
	}
	if(m_cycles % 28 == 0)
	{
		// 64kHz tick
		bool trigger  = false;
		auto numTicks = m_cycles / 28;
		auto freq1	  = m_RAM->DirectGet(ChipRegisters::AUDF1);
		if(freq1 && (irqen & 1) && numTicks % freq1 == 0)
		{
			m_irqStatus |= 1;
		}
		else
		{
			m_irqStatus &= ~1;
		}
		auto freq2 = m_RAM->DirectGet(ChipRegisters::AUDF2);
		if(freq2 && (irqen & 2) && numTicks % freq2 == 0)
		{
			m_irqStatus |= 2;
		}
		else
		{
			m_irqStatus &= ~2;
		}
		auto freq4 = m_RAM->DirectGet(ChipRegisters::AUDF4);
		if(freq4 && (irqen & 4) && numTicks % freq4 == 0)
		{
			m_irqStatus |= 4;
		}
		else
		{
			m_irqStatus &= ~4;
		}
		if(trigger)
		{
			m_CPU->IRQ();
		}
	}
}

byte_t PIA::Read(word_t addr)
{
	switch(addr)
	{
	case ChipRegisters::PORTA:
	{
		byte_t portA = 0xFF;
		if(m_joyUp)
		{
			portA &= ~0b1;
		}
		if(m_joyDown)
		{
			portA &= ~0b10;
		}
		if(m_joyLeft)
		{
			portA &= ~0b100;
		}
		if(m_joyRight)
		{
			portA &= ~0b1000;
		}
		return portA;
	}
	case ChipRegisters::PORTB:
	default:
		break;
	}

	return m_RAM->DirectGet(addr);
}

void PIA::Write(word_t addr, byte_t val)
{
	switch(addr)
	{
	case ChipRegisters::PORTB:
		m_RAM->DirectSet(addr, val | 1);
		break;
	default:
		m_RAM->DirectSet(addr, val);
		break;
	}
}

PIA::PIA(CPU* cpu, RAM* ram) : Chip(cpu, ram), m_joyUp(), m_joyDown(), m_joyLeft(), m_joyRight() {}

void PIA::Reset()
{
	// enable all ROMs by default
	m_RAM->DirectSet(ChipRegisters::PORTB, 0b00000001);
}

GTIA::GTIA(CPU* cpu, RAM* memory, ScanBuffer* scanBuffer) :
	Chip(cpu, memory), m_scanBuffer(scanBuffer), m_optionKey(), m_selectKey(), m_startKey(),
	m_joyFire(), m_collisions(), m_playerPos()
{
	memset(m_collisions, 0, sizeof(m_collisions));
	memset(m_playerPos, 0, sizeof(m_playerPos));
}

void GTIA::Tick()
{
	if(m_scanBuffer->scanCycle == 0)
	{
		memset(m_playerPos, 0, sizeof(m_playerPos));
	}

	if(m_RAM->DirectGet(ChipRegisters::GRACTL) & 0b10)
	{
		// player DMA
		DrawPlayer(ChipRegisters::HPOSP3,
				   ChipRegisters::COLPM3,
				   ChipRegisters::GRAFP3,
				   ChipRegisters::SIZEP3,
				   ChipRegisters::P3PF,
				   3);
		DrawPlayer(ChipRegisters::HPOSP2,
				   ChipRegisters::COLPM2,
				   ChipRegisters::GRAFP2,
				   ChipRegisters::SIZEP2,
				   ChipRegisters::P2PF,
				   2);
		DrawPlayer(ChipRegisters::HPOSP1,
				   ChipRegisters::COLPM1,
				   ChipRegisters::GRAFP1,
				   ChipRegisters::SIZEP1,
				   ChipRegisters::P1PF,
				   1);
		DrawPlayer(ChipRegisters::HPOSP0,
				   ChipRegisters::COLPM0,
				   ChipRegisters::GRAFP0,
				   ChipRegisters::SIZEP0,
				   ChipRegisters::P0PF,
				   0);
	}

	bool m0 = false, m1 = false, m2 = false, m3 = false;
	if(m_RAM->DirectGet(ChipRegisters::GRACTL) & 0b1)
	{
		// missile DMA
		m0 = DrawMissile(ChipRegisters::HPOSM0, ChipRegisters::COLPM0, 0, ChipRegisters::M0PF);
		m1 = DrawMissile(ChipRegisters::HPOSM1, ChipRegisters::COLPM1, 2, ChipRegisters::M1PF);
		m2 = DrawMissile(ChipRegisters::HPOSM2, ChipRegisters::COLPM2, 4, ChipRegisters::M2PF);
		m3 = DrawMissile(ChipRegisters::HPOSM3, ChipRegisters::COLPM3, 6, ChipRegisters::M3PF);
	}

	const auto distFromCenter = (m_scanBuffer->scanCycle * 2) - 0x80;
	const auto framePos		  = (FRAME_WIDTH / 2) + distFromCenter * 2;

	bool p0 = false, p1 = false, p2 = false, p3 = false;
	if(framePos >= 0 && framePos < FRAME_WIDTH)
	{
		p0 = m_playerPos[framePos] & (1 << 0);
		p1 = m_playerPos[framePos] & (1 << 1);
		p2 = m_playerPos[framePos] & (1 << 2);
		p3 = m_playerPos[framePos] & (1 << 3);
	}
	if(p0)
	{
		m_collisions[ChipRegisters::P0PL & 0xF] |= (p1 << 1);
		m_collisions[ChipRegisters::P0PL & 0xF] |= (p2 << 2);
		m_collisions[ChipRegisters::P0PL & 0xF] |= (p3 << 3);
	}
	if(p1)
	{
		m_collisions[ChipRegisters::P1PL & 0xF] |= (p0 << 0);
		m_collisions[ChipRegisters::P1PL & 0xF] |= (p2 << 2);
		m_collisions[ChipRegisters::P1PL & 0xF] |= (p3 << 3);
	}
	if(p2)
	{
		m_collisions[ChipRegisters::P2PL & 0xF] |= (p0 << 0);
		m_collisions[ChipRegisters::P2PL & 0xF] |= (p1 << 1);
		m_collisions[ChipRegisters::P2PL & 0xF] |= (p3 << 3);
	}
	if(p3)
	{
		m_collisions[ChipRegisters::P3PL & 0xF] |= (p0 << 0);
		m_collisions[ChipRegisters::P3PL & 0xF] |= (p1 << 1);
		m_collisions[ChipRegisters::P3PL & 0xF] |= (p2 << 2);
	}
	if(m0)
	{
		m_collisions[ChipRegisters::M0PL & 0xF] |= (p0 << 0);
		m_collisions[ChipRegisters::M0PL & 0xF] |= (p1 << 1);
		m_collisions[ChipRegisters::M0PL & 0xF] |= (p2 << 2);
		m_collisions[ChipRegisters::M0PL & 0xF] |= (p3 << 3);
	}
	if(m1)
	{
		m_collisions[ChipRegisters::M1PL & 0xF] |= (p0 << 0);
		m_collisions[ChipRegisters::M1PL & 0xF] |= (p1 << 1);
		m_collisions[ChipRegisters::M1PL & 0xF] |= (p2 << 2);
		m_collisions[ChipRegisters::M1PL & 0xF] |= (p3 << 3);
	}
	if(m2)
	{
		m_collisions[ChipRegisters::M2PL & 0xF] |= (p0 << 0);
		m_collisions[ChipRegisters::M2PL & 0xF] |= (p1 << 1);
		m_collisions[ChipRegisters::M2PL & 0xF] |= (p2 << 2);
		m_collisions[ChipRegisters::M2PL & 0xF] |= (p3 << 3);
	}
	if(m3)
	{
		m_collisions[ChipRegisters::M3PL & 0xF] |= (p0 << 0);
		m_collisions[ChipRegisters::M3PL & 0xF] |= (p1 << 1);
		m_collisions[ChipRegisters::M3PL & 0xF] |= (p2 << 2);
		m_collisions[ChipRegisters::M3PL & 0xF] |= (p3 << 3);
	}
}

bool GTIA::DrawMissile(word_t posRegister,
					   word_t colorRegister,
					   int	  shift,
					   word_t collisionRegister)
{
	const auto hPos = m_RAM->DirectGet(posRegister);

	bool missileDrawn = false;
	if(m_scanBuffer->scanCycle == hPos / 2)
	{
		const auto distFromCenter = hPos - 0x80;
		const auto framePos		  = (FRAME_WIDTH / 2) + distFromCenter * 2;
		const auto color		  = ANTIC::lookupColor(m_RAM->DirectGet(colorRegister));
		const auto bitMask		  = (m_RAM->DirectGet(ChipRegisters::GRAFM) >> shift) & 0b11;
		if(bitMask & 0b10)
		{
			missileDrawn						   = true;
			m_scanBuffer->lineBuffer[framePos]	   = color;
			m_scanBuffer->lineBuffer[framePos + 1] = color;
			if(m_scanBuffer->playfield[framePos])
			{
				m_collisions[collisionRegister & 0xF] |=
					(1 << (m_scanBuffer->playfield[framePos] - 1));
			}
			if(m_scanBuffer->playfield[framePos + 1])
			{
				m_collisions[collisionRegister & 0xF] |=
					(1 << (m_scanBuffer->playfield[framePos + 1] - 1));
			}
		}
		if(bitMask & 0b1)
		{
			missileDrawn						   = true;
			m_scanBuffer->lineBuffer[framePos + 2] = color;
			m_scanBuffer->lineBuffer[framePos + 3] = color;
			if(m_scanBuffer->playfield[framePos + 2])
			{
				m_collisions[collisionRegister & 0xF] |=
					(1 << (m_scanBuffer->playfield[framePos + 2] - 1));
			}
			if(m_scanBuffer->playfield[framePos + 3])
			{
				m_collisions[collisionRegister & 0xF] |=
					(1 << (m_scanBuffer->playfield[framePos + 3] - 1));
			}
		}
	}
	return missileDrawn;
}

bool GTIA::DrawPlayer(word_t posRegister,
					  word_t colorRegister,
					  word_t maskRegister,
					  word_t sizeRegister,
					  word_t collisionRegister,
					  int	 pNo)
{
	const auto hPos = m_RAM->DirectGet(posRegister);

	bool playerDrawn = false;
	if(m_scanBuffer->scanCycle == hPos / 2)
	{
		const auto distFromCenter = hPos - 0x80;
		const auto framePos		  = (FRAME_WIDTH / 2) + distFromCenter * 2;
		const auto color		  = ANTIC::lookupColor(m_RAM->DirectGet(colorRegister));

		auto bitMask = m_RAM->DirectGet(maskRegister);
		int	 size	 = 2;
		switch(m_RAM->DirectGet(sizeRegister))
		{
		case 1:
			size *= 2;
			break;
		case 3:
			size *= 4;
			break;
		}
		for(int i = 0; i < 8; i++)
		{
			for(int j = 0; j < size; j++)
			{
				if(bitMask & 1)
				{
					const auto offset = framePos + (7 - i) * size + j;
					if(offset >= 0 && offset < FRAME_WIDTH)
					{
						if(pNo > 0 && m_playerPos[offset] && m_playerPos[offset] & ((1 << pNo) - 1))
						{
							// don't draw if higher priorty player already drawn
						}
						else
						{
							m_scanBuffer->lineBuffer[offset] = color;
						}
						playerDrawn = true;
						if(m_scanBuffer->playfield[offset])
						{
							m_collisions[collisionRegister & 0xF] |=
								(1 << (m_scanBuffer->playfield[offset] - 1));
							m_playerPos[offset] |= (1 << pNo);
						}
					}
				}
			}
			bitMask >>= 1;
		}
	}
	return playerDrawn;
}
} // namespace atre
