#include "Memory.hpp"

namespace atre
{

byte_t Memory::Get(word_t addr) const
{
	return _bytes[addr];
}

void Memory::Set(word_t addr, byte_t val)
{
	_bytes[addr] = val;
}

word_t Memory::GetW(word_t addr) const
{
	// don't assume running on little-endian
	return static_cast<word_t>((_bytes[addr + 1] << 8) + _bytes[addr]);
}

void Memory::SetW(word_t addr, word_t val)
{
	_bytes[addr] = static_cast<byte_t>(val & 0xFF);
	_bytes[addr + 1] = static_cast<byte_t>(val >> 8);
}

void Memory::Load(const std::string &fileName, word_t startAddr)
{
	std::ifstream ifs(fileName, std::ios_base::binary);

	if (!ifs.good())
	{
		throw std::runtime_error("Unable to open file");
	}

	ifs.seekg(0, std::ios_base::end);
	auto size = ifs.tellg();
	ifs.seekg(0, std::ios_base::beg);

	ifs.read(reinterpret_cast<char *>(_bytes) + startAddr, size);

	ifs.close();
}

} // namespace atre
