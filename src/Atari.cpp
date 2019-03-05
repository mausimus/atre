#include "Atari.hpp"

namespace atre
{
void Atari::Reset()
{
	mMemory->Clear();
	mIO->Reset();
}

} // namespace atre
