#pragma once

#include "atre.hpp"

namespace atre
{

class Memory
{
	byte_t _bytes[MEM_SIZE];

  public:
	void Load(const std::string &fileName, word_t startAddr);

	byte_t Get(word_t addr) const;
	void Set(word_t addr, byte_t val);

	word_t GetW(word_t addr) const;
	void SetW(word_t addr, word_t val);
};

} // namespace atre
