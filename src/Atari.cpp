#include "Atari.hpp"

namespace atre
{
void Atari::Reset()
{
	mMemory->Clear();
	mCPU->Reset();
}
} // namespace atre
