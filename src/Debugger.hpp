#pragma once

#include "atre.hpp"

namespace atre
{
class Atari;
class IOPort;

class Debugger
{
	Atari *mAtari;

	std::atomic_bool mExiting;
	std::atomic_bool mStopping;
	std::mutex mRunningMutex;
	std::condition_variable mRunning;
	std::unique_ptr<std::thread> mCPUThread;

	void CPUThread();

  public:
	Debugger(Atari *atari);
	void Initialize();
	void Exit();

	// CPU callbacks
	void OnTrap();
	void OnIRQ();
	void OnNMI();
	void OnBRK();
	void OnReset();
	void OnBreak();

	void Dump();
	void DumpCallStack();
	void Start();
	void Stop();
	void Steps(bool);
	void Input(const std::string &command);
	void DumpReg(const std::string &fileName);
	void DumpMem(const std::string &fileName);
	void DumpDList();
	void BASIC();

	// interrupts
	void VBlank();
	void DList();
	void SerInt1();
	void SerInt2();
};
} // namespace atre
