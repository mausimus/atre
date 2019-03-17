#include "ANTIC.hpp"

using namespace std;

namespace atre
{

uint32_t ANTIC::_palette[128] = {0x00000000, 0x00101010, 0x00393939, 0x00636363, 0x007B7B7B, 0x00A5A5A5, 0x00C6C6C6, 0x00EFEFEF, 0x00100000, 0x00312100, 0x005A4200, 0x00846B00, 0x009C8400, 0x00C6AD00, 0x00E7D629, 0x00FFF74A, 0x00310000, 0x005A0800, 0x007B2900, 0x00A55200, 0x00BD6B00, 0x00E79429, 0x00FFB552, 0x00FFDE73, 0x004A0000, 0x006B0000, 0x00941000, 0x00BD3929, 0x00D65242, 0x00FF7B6B, 0x00FFA594, 0x00FFC6B5, 0x004A0000, 0x00730029, 0x0094004A, 0x00BD2973, 0x00D64294, 0x00FF6BB5, 0x00FF94DE, 0x00FFB5FF, 0x0039004A, 0x00630073, 0x008C0094, 0x00AD21BD, 0x00CE42D6, 0x00EF63FF, 0x00FF8CFF, 0x00FFB5FF, 0x0021007B, 0x004200A5, 0x006B00C6, 0x009429EF, 0x00AD42FF, 0x00D66BFF, 0x00F794FF, 0x00FFB5FF, 0x00000094, 0x002100BD, 0x004210DE, 0x006B39FF, 0x008452FF, 0x00AD7BFF, 0x00CE9CFF, 0x00F7C6FF, 0x0000008C, 0x000000B5, 0x001829D6, 0x004252FF, 0x005A6BFF, 0x008494FF, 0x00A5B5FF, 0x00CEDEFF, 0x00000063, 0x0000218C, 0x000042AD, 0x00186BD6, 0x003984F7, 0x005AADFF, 0x0084CEFF, 0x00ADF7FF, 0x00001021, 0x0000394A, 0x00005A73, 0x00008494, 0x00219CB5, 0x004AC6DE, 0x006BE7FF, 0x0094FFFF, 0x00002100, 0x00004A00, 0x00006B21, 0x0000944A, 0x0018AD6B, 0x0042D68C, 0x0063F7B5, 0x008CFFDE, 0x00002900, 0x00004A00, 0x00007300, 0x00109C08, 0x0029B521, 0x0052DE4A, 0x0073FF6B, 0x009CFF94, 0x00002100, 0x00004A00, 0x00006B00, 0x00299400, 0x0042AD00, 0x006BD610, 0x0094FF39, 0x00B5FF5A, 0x00001000, 0x00083900, 0x00296300, 0x00528400, 0x006BA500, 0x0094C600, 0x00B5EF18, 0x00DEFF42, 0x00080000, 0x00312100, 0x00524A00, 0x007B6B00, 0x00948C00, 0x00BDB500, 0x00E7D621, 0x00FFFF4A};

ANTIC::ANTIC(CPU *cpu, Memory *memory) : Chip(cpu, memory), _DLIs()
{
	memset(_frameBuffer, 0, FRAME_SIZE);
}

ANTIC::~ANTIC()
{
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

uint32_t ANTIC::GetRGB(byte_t color) const
{
	return _palette[color >> 1];
	//	auto lum = static_cast<uint32_t>((color & 0b1111) << 4);
	//	return (lum << 16) + (lum << 8) + lum;
}

void ANTIC::Blank(int numLines)
{
	if (numLines > 0)
	{
		while (numLines--)
		{
			auto bgColor = mMemory->Get(ChipRegisters::COLBK);
			auto lineStart = _frameBuffer + _y * 384;
			//		memset(lineStart, GetRGB(bgColor), 384 * 4);
			Fill(lineStart, GetRGB(bgColor), SCREEN_WIDTH);
			_y++;
		}
	}
}

void ANTIC::Fill(uint32_t *lineStart, uint32_t color, int width)
{
	while (width--)
	{
		*lineStart = color;
		lineStart++;
	}
}

void ANTIC::MapLine()
{
	auto bgColor = mMemory->Get(ChipRegisters::COLBK);
	auto outsideColor = GetRGB(bgColor);
	auto blankWidth = (384 - _width) / 2;
	if (_mode == 0xE)
	{
		auto linePtr = _frameBuffer + _y * 384;
		Fill(linePtr, outsideColor, blankWidth);

		for (int i = 0; i < 40; i++)
		{
			// each byte is 4 pixels, but each pixel is 2-wide
			// so each memory byte is 8 screen pixels
			byte_t pixelData = mMemory->Get(_lmsAddr + i);
			for (int b = 0; b < 4; b++)
			{
				byte_t colNo = pixelData & 0b11;
				uint32_t color;
				switch (colNo)
				{
				case 0:
					color = bgColor;
					break;
				case 1:
					color = GetRGB(mMemory->Get(ChipRegisters::COLPF0));
					break;
				case 2:
					color = GetRGB(mMemory->Get(ChipRegisters::COLPF1));
					break;
				case 3:
					color = GetRGB(mMemory->Get(ChipRegisters::COLPF2));
					break;
				}
				linePtr[blankWidth + i * 8 + (3 - b) * 2] = color;
				linePtr[blankWidth + i * 8 + (3 - b) * 2 + 1] = color;

				pixelData >>= 2;
			}
		}

		Fill(linePtr + 384 - blankWidth, outsideColor, blankWidth);
		_y++;
	}
}

void ANTIC::CharacterLine()
{
	auto bgColor = mMemory->Get(ChipRegisters::COLBK);
	auto tbColor = mMemory->Get(ChipRegisters::COLPF2);
	auto tfColor = mMemory->Get(ChipRegisters::COLPF1);
	auto foreColor = GetRGB((tbColor & 0b11110000) + (tfColor & 0b1111));
	auto backColor = GetRGB(tbColor);
	auto outsideColor = GetRGB(bgColor);
	word_t chBase = mMemory->Get(ChipRegisters::CHBASE) << 8;
	auto blinkState = mMemory->Get(ChipRegisters::CHACTL) & 0b11;
	auto isBlinkable = (_mode == 2 || _mode == 3);

	int bitWidth = 1;
	int bitHeight = 1;
	if (_mode == 6)
	{
		bitWidth = 2;
	}
	else if (_mode == 7)
	{
		bitWidth = 2;
		bitHeight = 2;
	}

	auto blankWidth = (384 - _width) / 2;
	auto charWidth = 8 * bitWidth;
	auto numChars = _width / charWidth;
	for (int l = 0; l < 8; l++)
	{
		for (int bh = 0; bh < bitHeight; bh++)
		{
			auto linePtr = _frameBuffer + _y * 384;
			Fill(linePtr, outsideColor, blankWidth);
			for (int n = 0; n < numChars; n++)
			{
				// line l of caracter n
				byte_t charNo = mMemory->Get(_lmsAddr + n);
				byte_t invert = 0;
				if (isBlinkable && (charNo & 0x80))
				{
					if (blinkState & 1)
					{
						continue; // hide hi chars
					}
					if (blinkState & 2)
					{
						invert = 1; // invert hi chars
					}
					charNo &= 0x7F;
				}
				else if (_mode == 6 || _mode == 7)
				{
					switch (_mode >> 6)
					{
					case 0:
						foreColor = GetRGB(mMemory->Get(ChipRegisters::COLPF0));
						break;
					case 1:
						foreColor = GetRGB(mMemory->Get(ChipRegisters::COLPF1));
						break;
					case 2:
						foreColor = GetRGB(mMemory->Get(ChipRegisters::COLPF2));
						break;
					case 3:
						foreColor = GetRGB(mMemory->Get(ChipRegisters::COLPF3));
						break;
					}
				}
				byte_t charLine = mMemory->Get(chBase + (charNo * 8) + l);
				// output bits as pixels
				for (int b = 0; b < 8; b++)
				{
					auto bit = (charLine & 1) ^ invert;
					auto color = bit ? foreColor : backColor;
					for (int bw = 0; bw < bitWidth; bw++)
					{
						linePtr[blankWidth + n * charWidth + (7 - b) * bitWidth + bw] = color;
					}
					charLine >>= 1;
				}
			}
			Fill(linePtr + 384 - blankWidth, outsideColor, blankWidth);
			_y++;
		}
	}
	_lmsAddr += numChars;
}

void ANTIC::RenderDisplayList()
{
	_y = TOP_SCANLINES;
	_DLIs.clear();

	if (!(mMemory->Get(ChipRegisters::DMACTL) & 0b100000))
	{
		return;
	}

	// playfield width (check on each scanline?)
	switch (mMemory->Get(ChipRegisters::DMACTL) & 0b11)
	{
	case 0:
		return;
	case 1:
		_width = PLAYFIELD_NARROW;
		break;
	case 2:
		_width = PLAYFIELD_NORMAL;
		break;
	case 3:
		_width = PLAYFIELD_WIDE;
		break;
	}

	word_t listStart = mMemory->GetW(ChipRegisters::DLISTL);
	word_t listAddr = listStart;
	_lmsAddr = 0;
	do
	{
		byte_t ai = mMemory->Get(listAddr);
		_mode = ai & 0b1111;
		bool DLI = ai & 0b10000000;
		bool LMS = ai & 0b01000000;
		//bool VS = ai & 0b00100000;
		//bool HS = ai & 0b00010000;
		switch (_mode)
		{
		case 0:
		{
			auto numBlanks = ((ai >> 4) & 0b111) + 1;
			Blank(numBlanks);
			listAddr++;
			break;
		}
		case 0x01:
		{
			auto jumpAddr = mMemory->GetW(listAddr + 1);
			listAddr = jumpAddr;
			if (LMS)
			{
				// blanks until the end
				Blank(VBLANK_SCANLINE - _y);
			}
			break;
		}
		default:
		{
			if (LMS)
			{
				_lmsAddr = mMemory->GetW(listAddr + 1);
				listAddr += 2;
			}
			if (_mode < 8)
			{
				CharacterLine();
			}
			else
			{
				MapLine();
			}
			listAddr++;
		}
		}
		if (DLI)
		{
			// trigger at the beginning of last scanline
			_DLIs.insert(_y - 1);
		}
	} while (listAddr != listStart && _y < VBLANK_SCANLINE);
}

void ANTIC::DrawFrame()
{
	RenderDisplayList();
}

void ANTIC::Tick()
{
	_lineCycle++;
	if (_lineCycle == CYCLES_PER_SCANLINE)
	{
		if (_DLIs.size())
		{
			DrawFrame(); // TO DO: make efficient

			// copy drawn line to screenbuffer
			memcpy(_screenBuffer + _lineNum, _frameBuffer + _lineNum,
				   SCREEN_WIDTH * 4);
		}

		_lineCycle = 0;
		_lineNum++;
		// _lineNum is starting drawing
		if (_DLIs.find(_lineNum) != _DLIs.end())
		{
			if (mMemory->DirectGet(ChipRegisters::NMIEN) & 128)
			{
				// trigger DLI at the beginning of this line
				mMemory->DirectSet(ChipRegisters::NMIST, 128);
				mCPU->NMI();
			}
		}
		if (_lineNum == VBLANK_SCANLINE)
		{
			_lineNum = 0;
			DrawFrame();
			memcpy(_screenBuffer, _frameBuffer,
				   FRAME_SIZE);

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
