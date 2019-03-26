#pragma once

#include "CPU.hpp"
#include "IO.hpp"
#include "RAM.hpp"

namespace atre
{
class Atari
{
	std::unique_ptr<RAM> m_RAM;
	std::unique_ptr<CPU> m_CPU;
	std::unique_ptr<IO>	 m_IO;

public:
	Atari();

	inline CPU* CPU()
	{
		return m_CPU.get();
	}

	inline IO* IO()
	{
		return m_IO.get();
	}

	inline RAM* RAM()
	{
		return m_RAM.get();
	}

	void Reset();
	void Boot(const std::string& osROM, const std::string& carridgeROM);
};
} // namespace atre
