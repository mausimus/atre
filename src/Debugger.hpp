#pragma once

#include "atre.hpp"

namespace atre
{
class Atari;

class Debugger
{
	Atari *mAtari;

	std::atomic_bool mExiting;
	std::atomic_bool mStopping;
	std::mutex mRunningMutex;
	std::condition_variable mRunning;
	std::unique_ptr<std::thread> mCPUThread;
	std::unique_ptr<std::thread> mIOThread;

	void CPUThread();
	void IOThread();

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
	void DumpReg(const std::string &fileName);
	void DumpMem(const std::string &fileName);
	void DumpDList();
};
} // namespace atre
