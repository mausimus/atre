#pragma once

#include "CPU.hpp"
#include "Memory.hpp"

namespace atre
{

class Atari
{
  public:
	Atari()
	{
		mMemory = std::make_unique<Memory>();
		mCPU = std::make_unique<CPU>(mMemory.get());
	}

	std::unique_ptr<CPU> mCPU;
	std::unique_ptr<Memory> mMemory;
};

} // namespace atre
