#include "Memory.hpp"
#include "Chips.hpp"

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
	_ioPorts.clear();
	_feedbackRegisters.clear();
}

void Memory::Connect(ChipIO *chipIO)
{
	_chipIO = chipIO;
}

void Memory::Move(word_t startAddr, word_t destAddr, word_t size)
{
	memcpy(reinterpret_cast<char *>(_bytes) + destAddr,
		   reinterpret_cast<char *>(_bytes) + startAddr, size);
	memset(reinterpret_cast<char *>(_bytes) + startAddr, 0, size);
}

void Memory::MapIOPort(word_t addr, shared_ptr<IOPort> ioPort)
{
	_ioPorts.insert(make_pair(addr, ioPort));
}

void Memory::MapFeedbackRegister(word_t addr, shared_ptr<FeedbackRegister> feedbackRegister)
{
	_feedbackRegisters.insert(make_pair(addr, feedbackRegister));
}

byte_t Memory::Get(word_t addr)
{
	if (addr >= 0xD000 && addr < 0xD800)
	{
		return _chipIO->Read(addr);
	}

	if (((addr >= 0xC000 && addr < 0xD000) || (addr >= 0xD800)) &&
		(DirectGet(ChipRegisters::PORTB) & 1)) // Kernel enabled
	{
		return _osROM[addr - 0xC000];
	}
	else if (addr >= 0xA000 && addr < 0xC000 && !(DirectGet(ChipRegisters::PORTB) & 2))
	{
		return _basicROM[addr - 0xA000];
	}
	else if (addr >= 0x5000 && addr < 0x5800 &&
			 !(DirectGet(ChipRegisters::PORTB) & 128) &&
			 (DirectGet(ChipRegisters::PORTB) & 1))
	{
		return _osROM[addr - 0x5000 + 0x1000];
	}

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

byte_t Memory::DirectGet(word_t addr)
{
	return _bytes[addr];
}

void Memory::DirectSet(word_t addr, byte_t val)
{
	_bytes[addr] = val;
}

void Memory::Set(word_t addr, byte_t val)
{
	if (addr >= 0xD000 && addr < 0xD800)
	{
		_chipIO->Write(addr, val);
		return;
	}

	if ((addr >= 0xC000 && addr < 0xD000) || (addr >= 0xD800))
	{
		if (!(DirectGet(ChipRegisters::PORTB) & 1)) // Kernel disabled
		{
			DirectSet(addr, val);
		}
		return;
	}
	if (addr >= 0xA000 && addr < 0xC000)
	{
		if (DirectGet(ChipRegisters::PORTB) & 2) // BASIC disabled
		{
			DirectSet(addr, val);
		}
		return;
	}
	if (addr >= 0x5000 && addr < 0x5800)
	{
		if (!(DirectGet(ChipRegisters::PORTB) & 1) ||
			DirectGet(ChipRegisters::PORTB) & 128) // self-test disabled
		{
			DirectSet(addr, val);
		}
		return;
	}

	if (_ioPorts.find(addr) != _ioPorts.end())
	{
		_ioPorts[addr]->queue.push_back(val);
	}
	if (_feedbackRegisters.find(addr) != _feedbackRegisters.end())
	{
		_feedbackRegisters[addr]->writeFunc(val);
	}

	_bytes[addr] = val;
}

word_t Memory::GetW(word_t addr)
{
	// don't assume running on little-endian
	return static_cast<word_t>((Get(addr + 1) << 8) + Get(addr));
}

void Memory::SetW(word_t addr, word_t val)
{
	Set(addr, static_cast<byte_t>(val & 0xFF));
	Set(addr + 1, static_cast<byte_t>(val >> 8));
}

void Memory::InternalLoad(const std::string &fileName, byte_t *addr)
{
	std::ifstream ifs(fileName, std::ios_base::binary);

	if (!ifs.good())
	{
		throw std::runtime_error("Unable to open file");
	}

	ifs.seekg(0, std::ios_base::end);
	auto size = ifs.tellg();
	ifs.seekg(0, std::ios_base::beg);

	ifs.read(reinterpret_cast<char *>(addr), size);

	ifs.close();
}

void Memory::Load(const std::string &fileName, word_t startAddr)
{
	InternalLoad(fileName, _bytes + startAddr);
}

void Memory::LoadROM(const std::string &osFileName, const std::string &basicFileName)
{
	InternalLoad(osFileName, _osROM);
	InternalLoad(basicFileName, _basicROM);
}

} // namespace atre
