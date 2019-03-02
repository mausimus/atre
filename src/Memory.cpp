#include "Memory.hpp"

using namespace std;

namespace atre
{

Memory::Memory() : _ioPorts()
{
	Clear();
}

void Memory::Clear()
{
	memset(_bytes, 0, MEM_SIZE);
}

void Memory::MapIOPort(word_t addr, IOPort *ioPort)
{
	_ioPorts.insert(make_pair(addr, ioPort));
}

byte_t Memory::Get(word_t addr)
{
	if (_ioPorts.find(addr) != _ioPorts.end())
	{
		auto ioPort = _ioPorts[addr];
		if (!ioPort->queue.empty())
		{
			auto v = ioPort->queue.front();
			ioPort->queue.pop_front();
			return v;
		}
		else if (ioPort->throwOnEmpty)
		{
			throw std::out_of_range("IOPort");
		}
		return 0;
	}

	return _bytes[addr];
}

void Memory::Set(word_t addr, byte_t val)
{
	if (_ioPorts.find(addr) != _ioPorts.end())
	{
		_ioPorts[addr]->queue.push_back(val);
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
