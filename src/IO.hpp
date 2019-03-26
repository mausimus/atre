#pragma once

#include "ANTIC.hpp"
#include "Chips.hpp"
#include <SDL.h>

namespace atre
{
class IOBase
{
protected:
	CPU* m_CPU;
	RAM* m_RAM;

public:
	IOBase(CPU* cpu, RAM* ram) : m_CPU(cpu), m_RAM(ram) {}

	virtual void Initialize() {}
	virtual void Refresh(bool) {}
	virtual void Destroy() {}

	virtual void Tick() {}
	virtual void Reset() {}
};

class IO : public IOBase
{
	static std::map<SDL_Scancode, byte_t> s_scanCodes;

	ANTIC m_ANTIC;
	GTIA  m_GTIA;
	POKEY m_POKEY;
	PIA	  m_PIA;

	SDL_Window*	  m_window;
	SDL_Renderer* m_renderer;
	SDL_Texture*  m_texture;

public:
	IO(CPU* cpu, RAM* ram);

	void Initialize() override;
	void Refresh(bool cpuRunning) override;
	void Destroy() override;

	void   Tick() override;
	void   Reset() override;
	void   Write(word_t reg, byte_t val);
	byte_t Read(word_t reg);
};
} // namespace atre
