#include "Chips.hpp"
#include "ANTIC.hpp"

using namespace std;

namespace atre
{
	byte_t GTIA::Read(word_t addr)
	{
		switch (addr)
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
			return _collisions[addr & 0xF];
		case ChipRegisters::PAL:
			return 0xF;
		case ChipRegisters::TRIG0:
			return !_joyFire;
		case ChipRegisters::TRIG1:
			return 0x1;
		case ChipRegisters::CONSOL:
		{
			byte_t consol = 0;
			if (!_optionKey)
			{
				consol += 1 << 2;
			}
			if (!_selectKey)
			{
				consol += 1 << 1;
			}
			if (!_startKey)
			{
				consol += 1;
			}
			return consol;
		}
		default:
			break;
		}

		return mMemory->DirectGet(addr);
	}

	void GTIA::Write(word_t addr, byte_t val)
	{
		switch (addr)
		{
		case ChipRegisters::HITCLR:
			memset(_collisions, 0, 16);
			break;
		case ChipRegisters::CONSOL:
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
			_irqStatus &= ~(0b01000000); // clear IRQ status once read??
			return _kbCode;
		case ChipRegisters::SKSTAT:
			return ~_kbStat;
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

	void POKEY::KeyDown(byte_t scanCode, bool shiftStatus, bool ctrlStatus)
	{
		_kbCode = scanCode;

		if (shiftStatus)
		{
			_kbCode |= 0b1000000;
			_kbStat |= 0b1000;
		}
		else
		{
			_kbStat &= ~(0b1000);
		}

		if (ctrlStatus)
		{
			_kbCode |= 0b10000000;
		}

		_kbStat |= 0b100;
		_keyPressed = true;
	}

	void POKEY::KeyUp()
	{
		_kbStat &= ~(0b100);
	}

	void POKEY::Break()
	{
		_breakPressed = true;
	}

	void POKEY::ShiftKey(bool shiftStatus)
	{
		if (shiftStatus)
		{
			_kbStat |= 0b1000;
		}
		else
		{
			_kbStat &= ~(0b1000);
		}
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
		if (_keyPressed)
		{
			if ((irqen & 0b1000000))
			{
				_irqStatus |= 0b1000000;
				mCPU->IRQ(); // keypress
			}
			_keyPressed = false;
		}
		if (_breakPressed)
		{
			if ((irqen & 0b10000000))
			{
				_irqStatus |= 0b10000000;
				mCPU->IRQ(); // break
			}
			_breakPressed = false;
		}

		_cycles++;
		if (_cycles == CYCLES_PER_SEC)
		{
			_cycles = 0;
		}
		if (_cycles % 28 == 0)
		{
			// 64kHz tick
			bool trigger = false;
			auto numTicks = _cycles / 28;
			auto freq1 = mMemory->DirectGet(ChipRegisters::AUDF1);
			if (freq1 && (irqen & 1) && numTicks % freq1 == 0)
			{
				_irqStatus |= 1;
			}
			else
			{
				_irqStatus &= ~1;
			}
			auto freq2 = mMemory->DirectGet(ChipRegisters::AUDF2);
			if (freq2 && (irqen & 2) && numTicks % freq2 == 0)
			{
				_irqStatus |= 2;
			}
			else
			{
				_irqStatus &= ~2;
			}
			auto freq4 = mMemory->DirectGet(ChipRegisters::AUDF4);
			if (freq4 && (irqen & 4) && numTicks % freq4 == 0)
			{
				_irqStatus |= 4;
			}
			else
			{
				_irqStatus &= ~4;
			}
			if (trigger)
			{
				mCPU->IRQ();
			}
		}
	}

	byte_t PIA::Read(word_t addr)
	{
		switch (addr)
		{
		case ChipRegisters::PORTA:
		{
			byte_t portA = 0xFF;
			if (_joyUp)
			{
				portA &= ~0b1;
			}
			if (_joyDown)
			{
				portA &= ~0b10;
			}
			if (_joyLeft)
			{
				portA &= ~0b100;
			}
			if (_joyRight)
			{
				portA &= ~0b1000;
			}
			return portA;
		}
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
			val |= 1;
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

	void GTIA::Tick()
	{
		_lineNum = _ANTIC->_lineNum;
		_lineCycle = _ANTIC->_lineCycle;
		_frameBuffer = _ANTIC->GetFrameBuffer();
		if (_lineCycle == 0)
		{
			memset(_playerPos, 0, FRAME_WIDTH);
		}

		bool m0 = false;
		bool m1 = false;
		bool m2 = false;
		bool m3 = false;
		if (mMemory->DirectGet(ChipRegisters::GRACTL) & 0b10)
		{
			// player DMA
			DrawPlayer(ChipRegisters::HPOSP3, ChipRegisters::COLPM3, ChipRegisters::GRAFP3, ChipRegisters::SIZEP3, ChipRegisters::P3PF, 3);
			DrawPlayer(ChipRegisters::HPOSP2, ChipRegisters::COLPM2, ChipRegisters::GRAFP2, ChipRegisters::SIZEP2, ChipRegisters::P2PF, 2);
			DrawPlayer(ChipRegisters::HPOSP1, ChipRegisters::COLPM1, ChipRegisters::GRAFP1, ChipRegisters::SIZEP1, ChipRegisters::P1PF, 1);
			DrawPlayer(ChipRegisters::HPOSP0, ChipRegisters::COLPM0, ChipRegisters::GRAFP0, ChipRegisters::SIZEP0, ChipRegisters::P0PF, 0);
		}
		if (mMemory->DirectGet(ChipRegisters::GRACTL) & 0b1)
		{
			// missile DMA
			m0 = DrawMissile(ChipRegisters::HPOSM0, ChipRegisters::COLPM0, 0, ChipRegisters::M0PF);
			m1 = DrawMissile(ChipRegisters::HPOSM1, ChipRegisters::COLPM1, 2, ChipRegisters::M1PF);
			m2 = DrawMissile(ChipRegisters::HPOSM2, ChipRegisters::COLPM2, 4, ChipRegisters::M2PF);
			m3 = DrawMissile(ChipRegisters::HPOSM3, ChipRegisters::COLPM3, 6, ChipRegisters::M3PF);
		}
		bool p0 = false, p1 = false, p2 = false, p3 = false;
		auto distFromCenter = (_lineCycle * 2) - 0x80;
		auto framePos = (FRAME_WIDTH / 2) + distFromCenter * 2;
		if (framePos >= 0 && framePos < FRAME_WIDTH)
		{
			p0 = _playerPos[framePos] & (1 << 0);
			p1 = _playerPos[framePos] & (1 << 1);
			p2 = _playerPos[framePos] & (1 << 2);
			p3 = _playerPos[framePos] & (1 << 3);
		}
		if (p0)
		{
			_collisions[ChipRegisters::P0PL & 0xF] |= (p1 << 1);
			_collisions[ChipRegisters::P0PL & 0xF] |= (p2 << 2);
			_collisions[ChipRegisters::P0PL & 0xF] |= (p3 << 3);
		}
		if (p1)
		{
			_collisions[ChipRegisters::P1PL & 0xF] |= (p0 << 0);
			_collisions[ChipRegisters::P1PL & 0xF] |= (p2 << 2);
			_collisions[ChipRegisters::P1PL & 0xF] |= (p3 << 3);
		}
		if (p2)
		{
			_collisions[ChipRegisters::P2PL & 0xF] |= (p0 << 0);
			_collisions[ChipRegisters::P2PL & 0xF] |= (p1 << 1);
			_collisions[ChipRegisters::P2PL & 0xF] |= (p3 << 3);
		}
		if (p3)
		{
			_collisions[ChipRegisters::P3PL & 0xF] |= (p0 << 0);
			_collisions[ChipRegisters::P3PL & 0xF] |= (p1 << 1);
			_collisions[ChipRegisters::P3PL & 0xF] |= (p2 << 2);
		}
		if (m0)
		{
			_collisions[ChipRegisters::M0PL & 0xF] |= (p0 << 0);
			_collisions[ChipRegisters::M0PL & 0xF] |= (p1 << 1);
			_collisions[ChipRegisters::M0PL & 0xF] |= (p2 << 2);
			_collisions[ChipRegisters::M0PL & 0xF] |= (p3 << 3);
		}
		if (m1)
		{
			_collisions[ChipRegisters::M1PL & 0xF] |= (p0 << 0);
			_collisions[ChipRegisters::M1PL & 0xF] |= (p1 << 1);
			_collisions[ChipRegisters::M1PL & 0xF] |= (p2 << 2);
			_collisions[ChipRegisters::M1PL & 0xF] |= (p3 << 3);
		}
		if (m2)
		{
			_collisions[ChipRegisters::M2PL & 0xF] |= (p0 << 0);
			_collisions[ChipRegisters::M2PL & 0xF] |= (p1 << 1);
			_collisions[ChipRegisters::M2PL & 0xF] |= (p2 << 2);
			_collisions[ChipRegisters::M2PL & 0xF] |= (p3 << 3);
		}
		if (m3)
		{
			_collisions[ChipRegisters::M3PL & 0xF] |= (p0 << 0);
			_collisions[ChipRegisters::M3PL & 0xF] |= (p1 << 1);
			_collisions[ChipRegisters::M3PL & 0xF] |= (p2 << 2);
			_collisions[ChipRegisters::M3PL & 0xF] |= (p3 << 3);
		}
	}

	bool GTIA::DrawMissile(word_t posRegister, word_t colorRegister, int shift, word_t collisionRegister)
	{
		bool missileDrawn = false;
		auto hPos = mMemory->DirectGet(posRegister);
		if (_lineCycle == hPos / 2)
		{
			// output missile 0 at horizonal position HPOSM0
			auto distFromCenter = hPos - 0x80;
			auto framePos = (FRAME_WIDTH / 2) + distFromCenter * 2;
			auto color = _ANTIC->GetRGB(mMemory->DirectGet(colorRegister));
			auto bitMask = (mMemory->DirectGet(ChipRegisters::GRAFM) >> shift) & 0b11;
			if (bitMask & 0b10)
			{
				missileDrawn = true;
				_frameBuffer[_lineNum * FRAME_WIDTH + framePos] = color;
				_frameBuffer[_lineNum * FRAME_WIDTH + framePos + 1] = color;
				if (_ANTIC->_linePlayfield[framePos])
				{
					_collisions[collisionRegister & 0xF] |= (1 << (_ANTIC->_linePlayfield[framePos] - 1));
				}
				if (_ANTIC->_linePlayfield[framePos + 1])
				{
					_collisions[collisionRegister & 0xF] |= (1 << (_ANTIC->_linePlayfield[framePos + 1] - 1));
				}
			}
			if (bitMask & 0b1)
			{
				missileDrawn = true;
				_frameBuffer[_lineNum * FRAME_WIDTH + framePos + 2] = color;
				_frameBuffer[_lineNum * FRAME_WIDTH + framePos + 3] = color;
				if (_ANTIC->_linePlayfield[framePos + 2])
				{
					_collisions[collisionRegister & 0xF] |= (1 << (_ANTIC->_linePlayfield[framePos + 2] - 1));
				}
				if (_ANTIC->_linePlayfield[framePos + 3])
				{
					_collisions[collisionRegister & 0xF] |= (1 << (_ANTIC->_linePlayfield[framePos + 3] - 1));
				}
			}
		}
		return missileDrawn;
	}

	bool GTIA::DrawPlayer(word_t posRegister, word_t colorRegister, word_t maskRegister, word_t sizeRegister, word_t collisionRegister, int pNo)
	{
		bool playerDrawn = false;
		auto hPos = mMemory->DirectGet(posRegister);
		if (_lineCycle == hPos / 2)
		{
			auto distFromCenter = hPos - 0x80;
			auto framePos = (FRAME_WIDTH / 2) + distFromCenter * 2;
			auto color = _ANTIC->GetRGB(mMemory->DirectGet(colorRegister));
			auto bitMask = mMemory->DirectGet(maskRegister);
			int size = 2;
			switch (mMemory->DirectGet(sizeRegister))
			{
			case 1:
				size *= 2;
				break;
			case 3:
				size *= 4;
				break;
			}
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < size; j++)
				{
					if (bitMask & 1)
					{
						auto offset = framePos + (7 - i) * size + j;
						if (offset >= 0 && offset < FRAME_WIDTH)
						{
							if (pNo > 0 && _playerPos[offset] && _playerPos[offset] & ((1 << pNo) - 1))
							{
								// don't draw if higher priorty player already drawn
							}
							else
							{
								_frameBuffer[_lineNum * FRAME_WIDTH + offset] = color;
							}
							playerDrawn = true;
							if (_ANTIC->_linePlayfield[offset])
							{
								_collisions[collisionRegister & 0xF] |= (1 << (_ANTIC->_linePlayfield[offset] - 1));
								_playerPos[offset] |= (1 << pNo);
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