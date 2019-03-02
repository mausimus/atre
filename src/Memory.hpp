#pragma once

#include "atre.hpp"

namespace atre
{

class IOPort
{
  public:
	IOPort() : queue(), throwOnEmpty(false)
	{
	}
	std::deque<byte_t> queue;
	bool throwOnEmpty;
};

class Memory
{
	byte_t _bytes[MEM_SIZE];
	std::map<word_t, IOPort *> _ioPorts;

  public:
	Memory();
	void Clear();

	void Load(const std::string &fileName, word_t startAddr);
	void MapIOPort(word_t addr, IOPort *ioPort);

	byte_t Get(word_t addr);
	void Set(word_t addr, byte_t val);

	word_t GetW(word_t addr) const;
	void SetW(word_t addr, word_t val);
};

} // namespace atre
