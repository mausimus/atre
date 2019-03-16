#include "ANTIC.hpp"

using namespace std;

namespace atre
{

ANTIC::ANTIC(CPU *cpu, Memory *memory) : Chip(cpu, memory), _window(), _renderer(), _texture()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	_window = SDL_CreateWindow("atre", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 384 * 2, 240 * 2, SDL_WINDOW_SHOWN);
	_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
	_texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ARGB8888,
								 SDL_TEXTUREACCESS_STREAMING, 384, 240);
	/*
	SDL_RendererInfo info;
	SDL_GetRendererInfo(_renderer, &info);
	cout << "Renderer name: " << info.name << endl;
	cout << "Texture formats: " << endl;
	for (Uint32 i = 0; i < info.num_texture_formats; i++)
	{
		cout << SDL_GetPixelFormatName(info.texture_formats[i]) << endl;
	}
	*/
}

ANTIC::~ANTIC()
{
	SDL_DestroyTexture(_texture);
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
	SDL_Quit();
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
	auto lum = static_cast<uint32_t>((color & 0b1111) << 4);
	return (lum << 16) + (lum << 8) + lum;
}

void ANTIC::Blank(int numLines)
{
	while (numLines--)
	{
		auto bgColor = mMemory->Get(ChipRegisters::COLBK);
		auto lineStart = _fb + _y * 384;
		memset(lineStart, GetRGB(bgColor), 384 * 4);
		_y++;
	}
}

void ANTIC::CharacterLine()
{
	auto bgColor = mMemory->Get(ChipRegisters::COLBK);
	auto tbColor = mMemory->Get(ChipRegisters::COLPF2);
	auto tfColor = mMemory->Get(ChipRegisters::COLPF1);
	word_t chBase = mMemory->Get(ChipRegisters::CHBASE);
	auto blinkState = mMemory->Get(ChipRegisters::CHACTL) & 0b11;
	chBase <<= 8;

	// render left blank
	auto blankWidth = (384 - _width) / 2;
	auto numChars = _width / 8;
	for (int l = 0; l < 8; l++)
	{
		auto linePtr = _fb + _y * 384;
		memset(linePtr, GetRGB(bgColor), blankWidth * 4);
		for (int n = 0; n < numChars; n++)
		{
			// line l of caracter n
			byte_t charNo = mMemory->Get(_lmsAddr + n);
			byte_t invert = 0;
			if (charNo & 0x80)
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
			byte_t charLine = mMemory->Get(chBase + (charNo * 8) + l);
			// output bits as pixels
			for (int b = 0; b < 8; b++)
			{
				auto bit = (charLine & 1) ^ invert;
				auto color = bit ? GetRGB(tfColor) : GetRGB(tbColor);
				linePtr[blankWidth + n * 8 + (8 - b)] = color;
				charLine >>= 1;
			}
		}
		memset(linePtr, GetRGB(bgColor), blankWidth * 4);
		_y++;
	}
	_lmsAddr += numChars;
}

void ANTIC::RenderDisplayList()
{
	_y = 0;

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
		_width = 256;
		break;
	case 2:
		_width = 320;
		break;
	case 3:
		_width = 384;
		break;
	}

	word_t listStart = mMemory->GetW(ChipRegisters::DLISTL);
	word_t listAddr = listStart;
	_lmsAddr = 0;
	do
	{
		byte_t ai = mMemory->Get(listAddr);
		_mode = ai & 0b1111;
		//bool DLI = ai & 0b10000000;
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
		case 0x41:
		{
			auto jumpAddr = mMemory->GetW(listAddr + 1);
			listAddr = jumpAddr;
			break;
		}
		default:
		{
			if (LMS)
			{
				_lmsAddr = mMemory->GetW(listAddr + 1);
				listAddr += 3;
				continue; // ? skip line with LMS?
			}
			if (_mode == 2)
			{
				CharacterLine();
			}
			listAddr++;
		}
		}
	} while (listAddr != listStart);
}

void ANTIC::DrawFrame()
{
	int pitch = 0;
	void *pixelsPtr;
	if (SDL_LockTexture(_texture, NULL, &pixelsPtr, &pitch))
	{
		throw runtime_error("Unable to lock texture");
	}
	_fb = reinterpret_cast<uint32_t *>(pixelsPtr);
	RenderDisplayList();
	SDL_UnlockTexture(_texture);
	SDL_RenderCopy(_renderer, _texture, NULL, NULL);
	SDL_RenderPresent(_renderer);
	SDL_PollEvent(NULL);
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
			DrawFrame();
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
