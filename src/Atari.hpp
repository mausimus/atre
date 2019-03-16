#pragma once

#include "CPU.hpp"
#include "Memory.hpp"
#include "Chips.hpp"

namespace atre
{

class Atari
{
  public:
	Atari();

	void Reset();

	std::unique_ptr<ChipIO> mIO;
	std::unique_ptr<CPU> mCPU;
	std::unique_ptr<Memory> mMemory;
};

} // namespace atre
