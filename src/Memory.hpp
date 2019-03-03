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

class FeedbackRegister
{
  public:
	FeedbackRegister(std::function<void(byte_t)> callback)
		: writeFunc(callback)
	{
	}
	std::function<void(byte_t)> writeFunc;
};

class Memory
{
	byte_t _bytes[MEM_SIZE];
	std::map<word_t, std::shared_ptr<IOPort>> _ioPorts;
	std::map<word_t, std::shared_ptr<FeedbackRegister>> _feedbackRegisters;

  public:
	Memory();
	void Clear();

	void Load(const std::string &fileName, word_t startAddr);
	void MapIOPort(word_t addr, std::shared_ptr<IOPort> ioPort);
	void MapFeedbackRegister(word_t addr, std::shared_ptr<FeedbackRegister> feedbackRegister);

	byte_t Get(word_t addr);
	void Set(word_t addr, byte_t val);

	word_t GetW(word_t addr) const;
	void SetW(word_t addr, word_t val);
};

} // namespace atre
