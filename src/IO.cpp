#include <SDL.h>

#include "IO.hpp"
#include "ANTIC.hpp"

using namespace std;

namespace atre
{
IO::IO(CPU *cpu, Memory *memory) : _GTIA(cpu, memory),
								   _ANTIC(cpu, memory),
								   _POKEY(cpu, memory),
								   _PIA(cpu, memory),
								   _CPU(cpu),
								   _window(), _renderer(), _texture()
{
}

void IO::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	_window = SDL_CreateWindow("atre", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
							   SCREEN_WIDTH * SCREEN_SCALE,
							   SCREEN_HEIGHT * SCREEN_SCALE,
							   SDL_WINDOW_SHOWN);

	_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
	_texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ARGB8888,
								 SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
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

map<SDL_Scancode, byte_t> IO::_scanCodes = {
	{SDL_SCANCODE_A, 0x3F},
	{SDL_SCANCODE_B, 0x15},
	{SDL_SCANCODE_C, 0x12},
	{SDL_SCANCODE_D, 0x3A},
	{SDL_SCANCODE_E, 0x2A},
	{SDL_SCANCODE_F, 0x38},
	{SDL_SCANCODE_G, 0x3D},
	{SDL_SCANCODE_H, 0x39},
	{SDL_SCANCODE_I, 0x0D},
	{SDL_SCANCODE_J, 0x01},
	{SDL_SCANCODE_K, 0x05},
	{SDL_SCANCODE_L, 0x00},
	{SDL_SCANCODE_M, 0x25},
	{SDL_SCANCODE_N, 0x23},
	{SDL_SCANCODE_O, 0x08},
	{SDL_SCANCODE_P, 0x0A},
	{SDL_SCANCODE_Q, 0x2F},
	{SDL_SCANCODE_R, 0x28},
	{SDL_SCANCODE_S, 0x3E},
	{SDL_SCANCODE_T, 0x2D},
	{SDL_SCANCODE_U, 0x0B},
	{SDL_SCANCODE_V, 0x10},
	{SDL_SCANCODE_W, 0x2E},
	{SDL_SCANCODE_X, 0x16},
	{SDL_SCANCODE_Y, 0x2B},
	{SDL_SCANCODE_Z, 0x17},
	{SDL_SCANCODE_0, 0x32},
	{SDL_SCANCODE_1, 0x1F},
	{SDL_SCANCODE_2, 0x1E},
	{SDL_SCANCODE_3, 0x1A},
	{SDL_SCANCODE_4, 0x18},
	{SDL_SCANCODE_5, 0x1D},
	{SDL_SCANCODE_6, 0x1B},
	{SDL_SCANCODE_7, 0x33},
	{SDL_SCANCODE_8, 0x35},
	{SDL_SCANCODE_9, 0x30},
	{SDL_SCANCODE_RETURN, 0x0C},
	{SDL_SCANCODE_ESCAPE, 0x1C},
	{SDL_SCANCODE_BACKSPACE, 0x34},
	{SDL_SCANCODE_TAB, 0x2C},
	{SDL_SCANCODE_SPACE, 0x21},
	{SDL_SCANCODE_MINUS, 0x0E},
	{SDL_SCANCODE_EQUALS, 0x0F},
	{SDL_SCANCODE_LEFTBRACKET, 0x36},
	{SDL_SCANCODE_RIGHTBRACKET, 0x37},
	{SDL_SCANCODE_BACKSLASH, 0x06},
	{SDL_SCANCODE_GRAVE, 0x07},
	{SDL_SCANCODE_SEMICOLON, 0x02},
	{SDL_SCANCODE_COMMA, 0x20},
	{SDL_SCANCODE_PERIOD, 0x22},
	{SDL_SCANCODE_SLASH, 0x26},
	{SDL_SCANCODE_CAPSLOCK, 0x3C},
	{SDL_SCANCODE_F1, 0x11}};

void IO::Refresh(bool cpuRunning)
{
	// update screen
	int pitch = 0;
	void *pixelsPtr;
	if (SDL_LockTexture(_texture, NULL, &pixelsPtr, &pitch))
	{
		throw runtime_error("Unable to lock texture");
	}
	memcpy(pixelsPtr, _ANTIC.GetFrameBuffer() + SCREEN_OFFSET, SCREEN_SIZE);
	SDL_UnlockTexture(_texture);
	SDL_RenderCopy(_renderer, _texture, NULL, NULL);
	SDL_RenderPresent(_renderer);

	if (cpuRunning)
	{
		// poll keyboard
		SDL_Event e;
		if (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				auto shiftStatus = e.key.keysym.mod & KMOD_SHIFT;
				_POKEY.ShiftKey(shiftStatus);
				if (e.key.keysym.scancode == SDL_SCANCODE_F2)
				{
					_GTIA.Start(e.type == SDL_KEYDOWN);
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F3)
				{
					_GTIA.Select(e.type == SDL_KEYDOWN);
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F4)
				{
					_GTIA.Option(e.type == SDL_KEYDOWN);
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F5)
				{
					_CPU->Reset();
				}
				else if (e.type == SDL_KEYUP)
				{
					_POKEY.KeyUp();
				}
				else
				{
					auto ctrlStatus = e.key.keysym.mod & KMOD_CTRL;
					auto scanCode = _scanCodes.find(e.key.keysym.scancode);
					if (scanCode != _scanCodes.end())
					{
						_POKEY.KeyDown(scanCode->second,
									   shiftStatus, ctrlStatus);
					}
					else if (e.key.keysym.scancode == SDL_SCANCODE_F6)
					{
						_POKEY.Break();
					}
				}
			}
			break;
			default:
				break;
			}
		}
	}
	else
	{
		SDL_PollEvent(NULL);
	}
}

void IO::Destroy()
{
	SDL_DestroyTexture(_texture);
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
	SDL_Quit();
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
	_ANTIC.Write(addr /*0xD400 + (addr & 0xF)*/, val);
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
	return _ANTIC.Read(0xD400 + (addr & 0xF));
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
	_ANTIC.Tick();
}

void IO::Reset()
{
	_GTIA.Reset();
	_POKEY.Reset();
	_PIA.Reset();
	_ANTIC.Reset();
}

} // namespace atre
