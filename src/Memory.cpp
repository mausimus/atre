#include "Memory.hpp"

using namespace std;

namespace atre
{

byte_t Memory::Get(word_t addr)
{
	static int redkar = 0;
	const string buffer{"C16386\r\n10 FOR I = 1 TO 10\r\n20 PRINT I\r\n30 NEXT I\r\nRUN\r\n"}; // TODO
	static unsigned int bint = 0;

	auto v = _bytes[addr];
	if (addr == 0xf004)
	{
		if (redkar == 1)
		{
			v = 0;
			redkar = 0;
		}
		else if (bint < buffer.length())
		{
			v = buffer[bint++];
			redkar = 1;
		}
		else
		{
			v = 0;
		}
	}
	return v;
}

void Memory::Set(word_t addr, byte_t val)
{
	if (addr == (word_t)0xf001)
	{
		cout << val << flush;
	}
	_bytes[addr] = val;
}

word_t Memory::GetW(word_t addr) const
{
	// don't assume running on little-endian
	return static_cast<word_t>((_bytes[addr + 1] << 8) + _bytes[addr]);
}

void Memory::SetW(word_t addr, word_t val)
{
	Set(addr, static_cast<byte_t>(val & 0xFF));
	Set(addr + 1, static_cast<byte_t>(val >> 8));
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
