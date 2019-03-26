#include "Chips.hpp"
#include "IO.hpp"
#include "RAM.hpp"

using namespace std;

namespace atre
{
RAM::RAM() : m_bytes(), m_osROM(), m_cartridgeROM(), m_feedbackRegisters(), m_IO()
{
	Clear();
}

void RAM::Clear()
{
	memset(m_bytes, 0, sizeof(m_bytes));
	memset(m_osROM, 0, sizeof(m_osROM));
	memset(m_cartridgeROM, 0, sizeof(m_cartridgeROM));
	m_feedbackRegisters.clear();
}

void RAM::Connect(IO* io)
{
	m_IO = io;
}

void RAM::Move(word_t startAddr, word_t destAddr, word_t size)
{
	memcpy(reinterpret_cast<char*>(m_bytes) + destAddr, reinterpret_cast<char*>(m_bytes) + startAddr, size);
	memset(reinterpret_cast<char*>(m_bytes) + startAddr, 0, size);
}

void RAM::MapFeedbackRegister(word_t addr, shared_ptr<FeedbackRegister> feedbackRegister)
{
	m_feedbackRegisters.insert(make_pair(addr, feedbackRegister));
}

byte_t RAM::Get(word_t addr)
{
	if(!m_IO)
	{
		return DirectGet(addr);
	}

	if(addr >= 0xD000 && addr < 0xD800)
	{
		return m_IO->Read(addr);
	}

	if(((addr >= 0xC000 && addr < 0xD000) || (addr >= 0xD800)) &&
	   (DirectGet(ChipRegisters::PORTB) & 1)) // Kernel enabled
	{
		return m_osROM[addr - 0xC000];
	}
	else if(addr >= 0xA000 && addr < 0xC000 && !(DirectGet(ChipRegisters::PORTB) & 2))
	{
		return m_cartridgeROM[addr - 0xA000];
	}
	else if(addr >= 0x5000 && addr < 0x5800 && !(DirectGet(ChipRegisters::PORTB) & 128) &&
			(DirectGet(ChipRegisters::PORTB) & 1))
	{
		return m_osROM[addr - 0x5000 + 0x1000];
	}

	return m_bytes[addr];
}

byte_t RAM::DirectGet(word_t addr)
{
	return m_bytes[addr];
}

void RAM::DirectSet(word_t addr, byte_t val)
{
	m_bytes[addr] = val;
}

void RAM::Set(word_t addr, byte_t val)
{
	if(m_feedbackRegisters.find(addr) != m_feedbackRegisters.end())
	{
		m_feedbackRegisters[addr]->writeFunc(val);
	}

	if(!m_IO)
	{
		DirectSet(addr, val);
		return;
	}

	if(addr >= 0xD000 && addr < 0xD800)
	{
		m_IO->Write(addr, val);
		return;
	}

	if((addr >= 0xC000 && addr < 0xD000) || (addr >= 0xD800))
	{
		if(!(DirectGet(ChipRegisters::PORTB) & 1)) // Kernel disabled
		{
			DirectSet(addr, val);
		}
		return;
	}
	if(addr >= 0xA000 && addr < 0xC000)
	{
		if(DirectGet(ChipRegisters::PORTB) & 2) // BASIC disabled
		{
			DirectSet(addr, val);
		}
		return;
	}
	if(addr >= 0x5000 && addr < 0x5800)
	{
		if(!(DirectGet(ChipRegisters::PORTB) & 1) || DirectGet(ChipRegisters::PORTB) & 128) // self-test disabled
		{
			DirectSet(addr, val);
		}
		return;
	}

	m_bytes[addr] = val;
}

word_t RAM::GetW(word_t addr)
{
	// don't assume running on little-endian
	return static_cast<word_t>((Get(addr + 1) << 8) + Get(addr));
}

void RAM::SetW(word_t addr, word_t val)
{
	Set(addr, static_cast<byte_t>(val & 0xFF));
	Set(addr + 1, static_cast<byte_t>(val >> 8));
}

void RAM::InternalLoad(const string& fileName, byte_t* addr)
{
	if(!fileName.length())
	{
		return;
	}
	ifstream ifs(fileName, ios_base::binary);

	if(!ifs.good())
	{
		throw runtime_error("Unable to open file");
	}

	ifs.seekg(0, ios_base::end);
	auto size = ifs.tellg();
	ifs.seekg(0, ios_base::beg);

	ifs.read(reinterpret_cast<char*>(addr), size);

	ifs.close();
}

void RAM::Load(const string& fileName, word_t startAddr)
{
	InternalLoad(fileName, m_bytes + startAddr);
}

void RAM::LoadROM(const string& osFileName, const string& cartridgeFileName)
{
	InternalLoad(osFileName, m_osROM);
	InternalLoad(cartridgeFileName, m_cartridgeROM);
}
} // namespace atre
