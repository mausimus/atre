#include "ANTIC.hpp"
#include "IO.hpp"
#include <SDL.h>

using namespace std;

namespace atre
{
const map<SDL_Scancode, int> IO::s_scanCodes = {
	{SDL_SCANCODE_A, 0x3F},			{SDL_SCANCODE_B, 0x15},			  {SDL_SCANCODE_C, 0x12},
	{SDL_SCANCODE_D, 0x3A},			{SDL_SCANCODE_E, 0x2A},			  {SDL_SCANCODE_F, 0x38},
	{SDL_SCANCODE_G, 0x3D},			{SDL_SCANCODE_H, 0x39},			  {SDL_SCANCODE_I, 0x0D},
	{SDL_SCANCODE_J, 0x01},			{SDL_SCANCODE_K, 0x05},			  {SDL_SCANCODE_L, 0x00},
	{SDL_SCANCODE_M, 0x25},			{SDL_SCANCODE_N, 0x23},			  {SDL_SCANCODE_O, 0x08},
	{SDL_SCANCODE_P, 0x0A},			{SDL_SCANCODE_Q, 0x2F},			  {SDL_SCANCODE_R, 0x28},
	{SDL_SCANCODE_S, 0x3E},			{SDL_SCANCODE_T, 0x2D},			  {SDL_SCANCODE_U, 0x0B},
	{SDL_SCANCODE_V, 0x10},			{SDL_SCANCODE_W, 0x2E},			  {SDL_SCANCODE_X, 0x16},
	{SDL_SCANCODE_Y, 0x2B},			{SDL_SCANCODE_Z, 0x17},			  {SDL_SCANCODE_0, 0x32},
	{SDL_SCANCODE_1, 0x1F},			{SDL_SCANCODE_2, 0x1E},			  {SDL_SCANCODE_3, 0x1A},
	{SDL_SCANCODE_4, 0x18},			{SDL_SCANCODE_5, 0x1D},			  {SDL_SCANCODE_6, 0x1B},
	{SDL_SCANCODE_7, 0x33},			{SDL_SCANCODE_8, 0x35},			  {SDL_SCANCODE_9, 0x30},
	{SDL_SCANCODE_RETURN, 0x0C},	{SDL_SCANCODE_ESCAPE, 0x1C},	  {SDL_SCANCODE_BACKSPACE, 0x34},
	{SDL_SCANCODE_TAB, 0x2C},		{SDL_SCANCODE_SPACE, 0x21},		  {SDL_SCANCODE_MINUS, 0x0E},
	{SDL_SCANCODE_EQUALS, 0x0F},	{SDL_SCANCODE_LEFTBRACKET, 0x36}, {SDL_SCANCODE_RIGHTBRACKET, 0x37},
	{SDL_SCANCODE_BACKSLASH, 0x06}, {SDL_SCANCODE_GRAVE, 0x07},		  {SDL_SCANCODE_SEMICOLON, 0x02},
	{SDL_SCANCODE_COMMA, 0x20},		{SDL_SCANCODE_PERIOD, 0x22},	  {SDL_SCANCODE_SLASH, 0x26},
	{SDL_SCANCODE_CAPSLOCK, 0x3C},	{SDL_SCANCODE_F1, 0x11}};

IO::IO(CPU* cpu, RAM* ram) :
	m_CPU(cpu), m_ANTIC(cpu, ram), m_GTIA(cpu, ram, m_ANTIC.getScanBuffer()), m_POKEY(cpu, ram), m_PIA(cpu, ram),
	m_window(), m_renderer(), m_texture()
{}

void IO::Initialize()
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	m_window = SDL_CreateWindow("atre",
								SDL_WINDOWPOS_UNDEFINED,
								SDL_WINDOWPOS_UNDEFINED,
								SCREEN_WIDTH * SCREEN_SCALE,
								SCREEN_HEIGHT * SCREEN_SCALE,
								SDL_WINDOW_SHOWN);

	m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
	m_texture  = SDL_CreateTexture(
		 m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	/*
	  SDL_RendererInfo info;
	  SDL_GetRendererInfo(m_renderer, &info);
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
	int	  pitch = 0;
	void* pixelsPtr;
	if(SDL_LockTexture(m_texture, NULL, &pixelsPtr, &pitch))
	{
		throw runtime_error("Unable to lock texture");
	}
	memcpy(pixelsPtr, m_ANTIC.getDisplayBuffer() + SCREEN_OFFSET, SCREEN_SIZE);
	SDL_UnlockTexture(m_texture);
	SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);
	SDL_RenderPresent(m_renderer);

	if(cpuRunning)
	{
		// poll keyboard
		SDL_Event e;
		if(SDL_PollEvent(&e))
		{
			switch(e.type)
			{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				auto shiftStatus = e.key.keysym.mod & KMOD_SHIFT;
				auto isUp		 = e.type == SDL_KEYUP;
				m_POKEY.ShiftKey(shiftStatus);
				if(e.key.keysym.scancode == SDL_SCANCODE_F2)
				{
					m_GTIA.Start(e.type == SDL_KEYDOWN);
				}
				else if(e.key.keysym.scancode == SDL_SCANCODE_F3)
				{
					m_GTIA.Select(e.type == SDL_KEYDOWN);
				}
				else if(e.key.keysym.scancode == SDL_SCANCODE_F4)
				{
					m_GTIA.Option(e.type == SDL_KEYDOWN);
				}
				else if(e.key.keysym.scancode == SDL_SCANCODE_F5)
				{
					m_CPU->Reset();
				}
				else if(e.key.keysym.scancode == SDL_SCANCODE_UP)
				{
					m_PIA.JoyUp(!isUp);
				}
				else if(e.key.keysym.scancode == SDL_SCANCODE_DOWN)
				{
					m_PIA.JoyDown(!isUp);
				}
				else if(e.key.keysym.scancode == SDL_SCANCODE_LEFT)
				{
					m_PIA.JoyLeft(!isUp);
				}
				else if(e.key.keysym.scancode == SDL_SCANCODE_RIGHT)
				{
					m_PIA.JoyRight(!isUp);
				}
				else if(e.key.keysym.scancode == SDL_SCANCODE_LCTRL)
				{
					m_GTIA.JoyFire(!isUp);
				}
				else if(isUp)
				{
					m_POKEY.KeyUp();
				}
				else
				{
					auto ctrlStatus = e.key.keysym.mod & KMOD_CTRL;
					auto scanCode	= s_scanCodes.find(e.key.keysym.scancode);
					if(scanCode != s_scanCodes.end())
					{
						m_POKEY.KeyDown(static_cast<byte_t>(scanCode->second), shiftStatus, ctrlStatus);
					}
					else if(e.key.keysym.scancode == SDL_SCANCODE_F6)
					{
						m_POKEY.Break();
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
	SDL_DestroyTexture(m_texture);
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

void IO::Write(word_t addr, byte_t val)
{
	if(addr < 0xD000 || addr >= 0xD800)
	{
		throw runtime_error("Invalid IO address");
	}
	if(addr < 0xD200)
	{
		m_GTIA.Write(addr, val);
		return;
	}
	if(addr < 0xD300)
	{
		m_POKEY.Write(addr, val);
		return;
	}
	if(addr < 0xD400)
	{
		m_PIA.Write(addr, val);
		return;
	}
	m_ANTIC.Write(addr, val);
}

byte_t IO::Read(word_t addr)
{
	if(addr < 0xD000 || addr >= 0xD800)
	{
		throw runtime_error("Invalid IO address");
	}
	if(addr < 0xD200)
	{
		return m_GTIA.Read(0xD000 + (addr & 0x1F));
	}
	if(addr < 0xD300)
	{
		return m_POKEY.Read(0xD200 + (addr & 0xF));
	}
	if(addr < 0xD400)
	{
		return m_PIA.Read(0xD300 + (addr & 0x3));
	}
	return m_ANTIC.Read(0xD400 + (addr & 0xF));
}

void IO::Tick()
{
	m_ANTIC.Tick();
	m_GTIA.Tick();
	m_POKEY.Tick();
	m_PIA.Tick();
}

void IO::Reset()
{
	m_GTIA.Reset();
	m_POKEY.Reset();
	m_PIA.Reset();
	m_ANTIC.Reset();
}
} // namespace atre
