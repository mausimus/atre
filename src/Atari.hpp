#pragma once

#include "CPU.hpp"
#include "IO.hpp"
#include "RAM.hpp"

namespace atre
{
class Atari
{
	std::unique_ptr<IO>	 m_IO;
	std::unique_ptr<CPU> m_CPU;
	std::unique_ptr<RAM> m_RAM;

public:
	Atari();

	inline CPU* CPU();
	inline IO*	IO();
	inline RAM* RAM();

	void Reset();
};
} // namespace atre
