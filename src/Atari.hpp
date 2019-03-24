#pragma once

#include "CPU.hpp"
#include "Memory.hpp"
#include "IO.hpp"

namespace atre
{
	class Atari
	{
	public:
		Atari();

		void Reset();

		std::unique_ptr<IO> mIO;
		std::unique_ptr<CPU> mCPU;
		std::unique_ptr<Memory> mMemory;
	};
} // namespace atre