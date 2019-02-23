#pragma once

#include "CPU.hpp"
#include "Memory.hpp"

namespace atre
{

class Atari
{
  public:
	Atari() : mCPU(mMemory), mMemory()
	{
	}

	CPU mCPU;
	Memory mMemory;
};

} // namespace atre
