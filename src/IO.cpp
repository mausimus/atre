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

void IO::Refresh(bool cpuRunning)
{
	// update screen
	int pitch = 0;
	void *pixelsPtr;
	if (SDL_LockTexture(_texture, NULL, &pixelsPtr, &pitch))
	{
		throw runtime_error("Unable to lock texture");
	}
	memcpy(pixelsPtr, _ANTIC.GetFrameBuffer(), FRAME_SIZE);
	SDL_UnlockTexture(_texture);
	SDL_RenderCopy(_renderer, _texture, NULL, NULL);
	SDL_RenderPresent(_renderer);

	if (cpuRunning)
	{
		// poll SDL key events
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
