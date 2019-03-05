#pragma once

#include "CPU.hpp"
#include "Memory.hpp"
#include "Chips.hpp"

namespace atre
{

class Atari
{
  public:
	Atari()
	{
		mMemory = std::make_unique<Memory>();
		mCPU = std::make_unique<CPU>(mMemory.get());
		mIO = std::make_unique<ChipIO>(mCPU.get(), mMemory.get());
		mMemory->Connect(mIO.get());
		mCPU->Connect(mIO.get());
	}

	void Reset();

	std::unique_ptr<ChipIO> mIO;
	std::unique_ptr<CPU> mCPU;
	std::unique_ptr<Memory> mMemory;
};

} // namespace atre
