#pragma once

#include <SDL.h>
#include "Chips.hpp"

namespace atre
{
class IO
{
  protected:
	GTIA _GTIA;
	std::unique_ptr<ANTIC> _ANTIC;
	POKEY _POKEY;
	PIA _PIA;

  public:
	void KeyboardInput(const std::string &input);

	IO(CPU *cpu, Memory *memory);
	void Tick();
	void Reset();
	void Write(word_t reg, byte_t val);
	byte_t Read(word_t reg);
};
} // namespace atre
