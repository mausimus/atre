#include "Atari.hpp"
#include "Chips.hpp"
#include "ANTIC.hpp"

namespace atre
{

Atari::Atari()
{
	mMemory = std::make_unique<Memory>();
	mCPU = std::make_unique<CPU>(mMemory.get());
	mIO = std::make_unique<ChipIO>(mCPU.get(), mMemory.get());
	mMemory->Connect(mIO.get());
	mCPU->Connect(mIO.get());
}

void Atari::Reset()
{
	mMemory->Clear();
	mIO->Reset();
}

} // namespace atre
