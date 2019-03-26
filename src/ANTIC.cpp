#include "ANTIC.hpp"

using namespace std;

namespace atre
{
const uint32_t ANTIC::s_palette[128] = {
	0x00000000, 0x00101010, 0x00393939, 0x00636363, 0x007B7B7B, 0x00A5A5A5, 0x00C6C6C6, 0x00EFEFEF, 0x00100000,
	0x00312100, 0x005A4200, 0x00846B00, 0x009C8400, 0x00C6AD00, 0x00E7D629, 0x00FFF74A, 0x00310000, 0x005A0800,
	0x007B2900, 0x00A55200, 0x00BD6B00, 0x00E79429, 0x00FFB552, 0x00FFDE73, 0x004A0000, 0x006B0000, 0x00941000,
	0x00BD3929, 0x00D65242, 0x00FF7B6B, 0x00FFA594, 0x00FFC6B5, 0x004A0000, 0x00730029, 0x0094004A, 0x00BD2973,
	0x00D64294, 0x00FF6BB5, 0x00FF94DE, 0x00FFB5FF, 0x0039004A, 0x00630073, 0x008C0094, 0x00AD21BD, 0x00CE42D6,
	0x00EF63FF, 0x00FF8CFF, 0x00FFB5FF, 0x0021007B, 0x004200A5, 0x006B00C6, 0x009429EF, 0x00AD42FF, 0x00D66BFF,
	0x00F794FF, 0x00FFB5FF, 0x00000094, 0x002100BD, 0x004210DE, 0x006B39FF, 0x008452FF, 0x00AD7BFF, 0x00CE9CFF,
	0x00F7C6FF, 0x0000008C, 0x000000B5, 0x001829D6, 0x004252FF, 0x005A6BFF, 0x008494FF, 0x00A5B5FF, 0x00CEDEFF,
	0x00000063, 0x0000218C, 0x000042AD, 0x00186BD6, 0x003984F7, 0x005AADFF, 0x0084CEFF, 0x00ADF7FF, 0x00001021,
	0x0000394A, 0x00005A73, 0x00008494, 0x00219CB5, 0x004AC6DE, 0x006BE7FF, 0x0094FFFF, 0x00002100, 0x00004A00,
	0x00006B21, 0x0000944A, 0x0018AD6B, 0x0042D68C, 0x0063F7B5, 0x008CFFDE, 0x00002900, 0x00004A00, 0x00007300,
	0x00109C08, 0x0029B521, 0x0052DE4A, 0x0073FF6B, 0x009CFF94, 0x00002100, 0x00004A00, 0x00006B00, 0x00299400,
	0x0042AD00, 0x006BD610, 0x0094FF39, 0x00B5FF5A, 0x00001000, 0x00083900, 0x00296300, 0x00528400, 0x006BA500,
	0x0094C600, 0x00B5EF18, 0x00DEFF42, 0x00080000, 0x00312100, 0x00524A00, 0x007B6B00, 0x00948C00, 0x00BDB500,
	0x00E7D621, 0x00FFFF4A};

ANTIC::ANTIC(CPU* cpu, RAM* ram) :
	Chip(cpu, ram), m_renderBuffer(), m_displayBuffer(), m_renderLine(), m_displayMode(), m_playfieldWidth(),
	m_scanAddress(), m_listAddress(), m_triggerDLI(), m_hScroll(), m_listActive(), m_lastFrameTime(), m_scanLine(),
	m_scanBuffer()
{
	memset(m_renderBuffer, 0, FRAME_BYTES);
	memset(m_displayBuffer, 0, FRAME_BYTES);
	memset(m_scanBuffer.playfield, 0, FRAME_WIDTH);
	m_scanBuffer.lineBuffer = m_renderBuffer;
}

ANTIC::~ANTIC() {}

byte_t ANTIC::Read(word_t addr)
{
	switch(addr)
	{
	case ChipRegisters::VCOUNT:
	{
		return static_cast<byte_t>(m_scanLine / 2);
	}
	case ChipRegisters::NMIST:
	default:
		break;
	}

	return m_RAM->DirectGet(addr);
}

void ANTIC::Write(word_t addr, byte_t val)
{
	switch(addr)
	{
	case ChipRegisters::WSYNC:
		m_RAM->DirectSet(addr, val);
		// wait until cycle 105 of current scanline
		if(m_scanBuffer.scanCycle < 104)
		{
			m_CPU->Wait(104 - m_scanBuffer.scanCycle);
		}
		else
		{
			m_CPU->Wait(CYCLES_PER_SCANLINE + 104 - m_scanBuffer.scanCycle);
		}
		break;
	case ChipRegisters::NMIRES:
		m_RAM->DirectSet(ChipRegisters::NMIST, 0);
		return;
	case ChipRegisters::NMIEN:
	default:
		m_RAM->DirectSet(addr, val);
		break;
	}
}

void ANTIC::Reset()
{
	m_scanLine			   = 0;
	m_scanBuffer.scanCycle = 0;
	m_RAM->DirectSet(ChipRegisters::NMIEN, 0);
	m_RAM->DirectSet(ChipRegisters::NMIST, 0);
}

uint32_t ANTIC::lookupColor(byte_t color)
{
	return s_palette[color >> 1];
}

const uint32_t* ANTIC::getDisplayBuffer() const
{
	return m_displayBuffer;
}

ScanBuffer* ANTIC::getScanBuffer()
{
	return &m_scanBuffer;
}

void ANTIC::renderBlankLines(int numLines)
{
	while(numLines-- > 0)
	{
		const auto bgColor	 = m_RAM->Get(ChipRegisters::COLBK);
		const auto lineStart = m_renderBuffer + m_renderLine * FRAME_WIDTH;
		fill(lineStart, lookupColor(bgColor), FRAME_WIDTH);
		m_renderLine++;
	}
}

void ANTIC::fill(uint32_t* lineStart, uint32_t color, int width)
{
	while(width--)
	{
		*lineStart = color;
		lineStart++;
	}
}

void ANTIC::renderMapLine()
{
	const auto bgColor		= m_RAM->Get(ChipRegisters::COLBK);
	const auto outsideColor = lookupColor(bgColor);
	const auto blankWidth	= (FRAME_WIDTH - m_playfieldWidth) / 2;
	memset(m_scanBuffer.playfield, 0, FRAME_WIDTH);

	auto pixelWidth	  = 1;
	auto pixelHeight  = 1;
	auto bitsPerPixel = 1;
	switch(m_displayMode)
	{
	case 0xE:
		pixelWidth	 = 2;
		bitsPerPixel = 2;
		break;
	case 0x8:
		pixelWidth	 = 8;
		pixelHeight	 = 8;
		bitsPerPixel = 2;
		break;
	case 0xB:
		pixelHeight	 = 2;
		pixelWidth	 = 2;
		bitsPerPixel = 1;
		break;
	default:
		break;
	}

	const auto bytesPerLine	 = static_cast<word_t>((m_playfieldWidth / pixelWidth) * bitsPerPixel / 8);
	const auto widthPerByte	 = m_playfieldWidth / bytesPerLine;
	const auto pixelsPerByte = 8 / bitsPerPixel;
	for(auto py = 0; py < pixelHeight; py++)
	{
		const auto linePtr = m_renderBuffer + m_renderLine * FRAME_WIDTH;
		fill(linePtr, outsideColor, blankWidth);
		for(word_t i = 0; i < bytesPerLine; i++)
		{
			byte_t pixelData = m_RAM->Get(m_scanAddress + i);
			for(auto b = 0; b < pixelsPerByte; b++)
			{
				const byte_t colorIndex = bitsPerPixel == 2 ? (pixelData & 0b11) : (pixelData & 1);
				uint32_t	 colorValue;
				switch(colorIndex)
				{
				case 1:
					colorValue = lookupColor(m_RAM->Get(ChipRegisters::COLPF0));
					break;
				case 2:
					colorValue = lookupColor(m_RAM->Get(ChipRegisters::COLPF1));
					break;
				case 3:
					colorValue = lookupColor(m_RAM->Get(ChipRegisters::COLPF2));
					break;
				default:
					colorValue = lookupColor(bgColor);
					break;
				}
				for(auto px = 0; px < pixelWidth; px++)
				{
					const auto offset = blankWidth + i * widthPerByte + (pixelsPerByte - 1 - b) * pixelWidth + px;
					linePtr[offset]	  = colorValue;
					m_scanBuffer.playfield[offset] = colorIndex;
				}

				pixelData >>= bitsPerPixel;
			}
		}

		fill(linePtr + FRAME_WIDTH - blankWidth, outsideColor, blankWidth);
		m_renderLine++;
	}
	m_scanAddress += bytesPerLine;
}

void ANTIC::renderCharacterLine()
{
	const auto	 backgroundColor = m_RAM->Get(ChipRegisters::COLBK);
	const auto	 textBgColor	 = m_RAM->Get(ChipRegisters::COLPF2);
	const auto	 textFgColor	 = m_RAM->Get(ChipRegisters::COLPF1);
	const auto	 outsideColor	 = lookupColor(backgroundColor);
	const word_t characterMap	 = m_RAM->Get(ChipRegisters::CHBASE) << 8;
	const auto	 blinkState		 = m_RAM->Get(ChipRegisters::CHACTL) & 0b11;

	auto bitWidth  = 1;
	auto bitHeight = 1;
	switch(m_displayMode)
	{
	case 6:
		bitWidth = 2;
		break;
	case 7:
		bitWidth  = 2;
		bitHeight = 2;
		break;
	}

	const auto blankWidth	 = (FRAME_WIDTH - m_playfieldWidth) / 2;
	const auto charWidth	 = 8 * bitWidth;
	const auto charsPerLine	 = static_cast<word_t>(m_playfieldWidth / charWidth);
	const auto hScrollAmount = m_hScroll ? m_RAM->DirectGet(ChipRegisters::HSCROL) * 2 : 0;

	auto foreColor = lookupColor((textBgColor & 0b11110000) + (textFgColor & 0b1111));
	auto backColor = lookupColor(textBgColor);

	for(auto charLine = 0; charLine < 8; charLine++)
	{
		for(auto by = 0; by < bitHeight; by++)
		{
			const auto linePtr = m_renderBuffer + m_renderLine * FRAME_WIDTH;
			fill(linePtr, outsideColor, blankWidth);
			for(word_t n = 0; n < charsPerLine; n++)
			{
				// line l of character n
				byte_t charNo = m_RAM->Get(m_scanAddress + n);
				bool   invert = false;
				switch(m_displayMode)
				{
				case 2:
				case 3:
					if(charNo & 0x80)
					{
						if(blinkState & 1)
						{
							continue; // hide hi chars
						}
						if(blinkState & 2)
						{
							invert = 1; // invert hi chars
						}
						charNo &= 0x7F;
					}
					break;
				case 6:
				case 7:
					backColor = outsideColor;
					switch(charNo >> 6)
					{
					case 0:
						foreColor = lookupColor(m_RAM->Get(ChipRegisters::COLPF0));
						break;
					case 1:
						foreColor = lookupColor(m_RAM->Get(ChipRegisters::COLPF1));
						break;
					case 2:
						foreColor = lookupColor(m_RAM->Get(ChipRegisters::COLPF2));
						break;
					case 3:
						foreColor = lookupColor(m_RAM->Get(ChipRegisters::COLPF3));
						break;
					}
					charNo &= 0b111111;
					break;
				default:
					break;
				}
				byte_t charSlice = m_RAM->Get(static_cast<word_t>(characterMap + (charNo * 8) + charLine));
				// output bits as pixels
				for(auto bitNo = 0; bitNo < 8; bitNo++)
				{
					const auto bit	 = (charSlice & 1) ^ invert;
					const auto color = bit ? foreColor : backColor;
					for(auto bx = 0; bx < bitWidth; bx++)
					{
						const auto lineOffset = hScrollAmount + n * charWidth + (7 - bitNo) * bitWidth + bx;
						if(lineOffset < m_playfieldWidth)
						{
							linePtr[blankWidth + lineOffset] = color;
						}
					}
					charSlice >>= 1;
				}
			}
			fill(linePtr + FRAME_WIDTH - blankWidth, outsideColor, blankWidth);
			m_renderLine++;
		}
	}
	m_scanAddress += charsPerLine;
}

void ANTIC::startDisplayList()
{
	m_renderLine = TOP_SCANLINES;

	if(!(m_RAM->Get(ChipRegisters::DMACTL) & 0b100000))
	{
		m_listActive = false;
		return;
	}

	m_listAddress = m_RAM->GetW(ChipRegisters::DLISTL);
	m_scanAddress = 0;
	m_triggerDLI  = false;
	m_listActive  = true;
}

void ANTIC::stepDisplayList()
{
	m_triggerDLI = false;

	if(!m_listActive)
	{
		return;
	}
	if(!(m_RAM->Get(ChipRegisters::DMACTL) & 0b100000))
	{
		m_listActive = false;
		return;
	}
	if(m_renderLine >= VBLANK_SCANLINE)
	{
		return;
	}

	switch(m_RAM->Get(ChipRegisters::DMACTL) & 0b11)
	{
	case 0:
		return;
	case 1:
		m_playfieldWidth = PLAYFIELD_NARROW;
		break;
	case 2:
		m_playfieldWidth = PLAYFIELD_NORMAL;
		break;
	case 3:
		m_playfieldWidth = PLAYFIELD_WIDE;
		break;
	}

	const byte_t instr = m_RAM->Get(m_listAddress);
	const bool	 LMS   = instr & 0b01000000;
	m_displayMode	   = instr & 0b1111;
	m_triggerDLI	   = instr & 0b10000000;
	m_hScroll		   = instr & 0b00010000;
	switch(m_displayMode)
	{
	case 0:
	{
		const auto numBlanks = ((instr >> 4) & 0b111) + 1;
		renderBlankLines(numBlanks);
		m_listAddress++;
		break;
	}
	case 0x01:
	{
		m_listAddress = m_RAM->GetW(m_listAddress + 1);
		if(LMS)
		{
			// blanks until the end
			renderBlankLines(VBLANK_SCANLINE - m_renderLine);
		}
		break;
	}
	default:
	{
		if(LMS)
		{
			m_scanAddress = m_RAM->GetW(m_listAddress + 1);
			m_listAddress += 2;
		}
		if(m_displayMode < 8)
		{
			renderCharacterLine();
		}
		else
		{
			renderMapLine();
		}
		m_listAddress++;
	}
	}
}

void ANTIC::Tick()
{
	m_scanBuffer.scanCycle++;
	if(m_scanBuffer.scanCycle == CYCLES_PER_SCANLINE)
	{
		m_scanLine++;
		m_scanBuffer.scanCycle	= 0;
		m_scanBuffer.lineBuffer = m_renderBuffer + m_scanLine * FRAME_WIDTH;

		// m_scanLine is starting tracing, m_renderLine is the first line we haven't
		// actually drawn yet
		if(m_scanLine >= m_renderLine)
		{
			// catch up drawing with tracing
			stepDisplayList();
		}
		// if we've started tracing the last line of the instruction and there was a
		// DLI request, trigger
		if(m_renderLine > 0 && m_scanLine == m_renderLine - 1 && m_triggerDLI)
		{
			if(m_RAM->DirectGet(ChipRegisters::NMIEN) & 128)
			{
				// trigger DLI at the beginning of this line
				m_RAM->DirectSet(ChipRegisters::NMIST, 128);
				m_CPU->NMI();
			}
		}

		if(m_scanLine == VBLANK_SCANLINE)
		{
			memcpy(m_displayBuffer, m_renderBuffer, FRAME_BYTES);
			if(m_RAM->DirectGet(ChipRegisters::NMIEN) & 64)
			{
				// VBlank interrupt
				m_RAM->DirectSet(ChipRegisters::NMIST, 64);
				m_CPU->NMI();
			}
		}
		else if(m_scanLine == TOTAL_SCANLINES)
		{
			m_scanLine = 0;
			startDisplayList();

			const chrono::duration<double> frameTime = chrono::steady_clock::now() - m_lastFrameTime;
			if(frameTime.count() < FRAME_TIME)
			{
				// slow down to real time
				this_thread::sleep_for(chrono::duration<double>(FRAME_TIME - frameTime.count()));
			}
			m_lastFrameTime = chrono::steady_clock::now();
		}

		// P/M if DMA enabled
		if(m_scanLine < VBLANK_SCANLINE && m_RAM->DirectGet(ChipRegisters::DMACTL) & 0b1000)
		{
			const word_t pmGraphicsBase = m_RAM->DirectGet(ChipRegisters::PMBASE) << 8;
			const bool	 lowResolution	= !(m_RAM->DirectGet(ChipRegisters::DMACTL) & 0b10000);
			const auto	 sectionLength	= lowResolution ? 128 : 256;
			const auto	 sectionOffset	= lowResolution ? m_scanLine / 2 : m_scanLine;
			m_RAM->DirectSet(ChipRegisters::GRAFM,
							 m_RAM->DirectGet(static_cast<word_t>(pmGraphicsBase + sectionLength * 3 + sectionOffset)));
			m_RAM->DirectSet(ChipRegisters::GRAFP0,
							 m_RAM->DirectGet(static_cast<word_t>(pmGraphicsBase + sectionLength * 4 + sectionOffset)));
			m_RAM->DirectSet(ChipRegisters::GRAFP1,
							 m_RAM->DirectGet(static_cast<word_t>(pmGraphicsBase + sectionLength * 5 + sectionOffset)));
			m_RAM->DirectSet(ChipRegisters::GRAFP2,
							 m_RAM->DirectGet(static_cast<word_t>(pmGraphicsBase + sectionLength * 6 + sectionOffset)));
			m_RAM->DirectSet(ChipRegisters::GRAFP3,
							 m_RAM->DirectGet(static_cast<word_t>(pmGraphicsBase + sectionLength * 7 + sectionOffset)));
		}
	}
}
} // namespace atre
