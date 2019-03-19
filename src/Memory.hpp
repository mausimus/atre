#pragma once

#include "atre.hpp"

namespace atre
{

class IO;

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
	friend class Debugger;

	byte_t _bytes[MEM_SIZE];
	byte_t _osROM[16386];
	byte_t _basicROM[8192];

	std::map<word_t, std::shared_ptr<FeedbackRegister>> _feedbackRegisters;
	IO *_IO;

	void InternalLoad(const std::string &fileName, byte_t *addr);

  public:
	Memory();
	void Clear();
	void Move(word_t startAddr, word_t destAddr, word_t size);
	void Connect(IO *io);

	void Load(const std::string &fileName, word_t startAddr);
	void LoadROM(const std::string &osFileName, const std::string &basicFileName);
	void MapFeedbackRegister(word_t addr, std::shared_ptr<FeedbackRegister> feedbackRegister);

	virtual byte_t Get(word_t addr);
	virtual void Set(word_t addr, byte_t val);
	byte_t DirectGet(word_t addr);
	void DirectSet(word_t addr, byte_t val);

	word_t GetW(word_t addr);
	void SetW(word_t addr, word_t val);
};

class DirectMemory : public Memory
{
  public:
	DirectMemory() : Memory() {}

	byte_t Get(word_t addr) override { return DirectGet(addr); }
	void Set(word_t addr, byte_t val) override { DirectSet(addr, val); }
};

} // namespace atre
