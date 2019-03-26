#pragma once

#include "atre.hpp"

namespace atre
{
class Atari;

class Debugger
{
	Atari* mAtari;

	std::atomic_bool			 m_exiting;
	std::atomic_bool			 m_stopping;
	std::mutex					 m_mutex;
	std::condition_variable		 m_running;
	std::unique_ptr<std::thread> m_CPUThread;
	std::unique_ptr<std::thread> m_IOThread;

	void CPUThread();
	void IOThread();

public:
	Debugger(Atari* atari);

	void Initialize();
	void Exit();

	// CPU callbacks
	void OnTrap();
	void OnIRQ();
	void OnNMI();
	void OnBRK();
	void OnReset();
	void OnBreak();

	// commands
	void DumpState();
	void CallStack();
	void Start();
	void Stop();
	void Steps(bool);
	void DumpRAM(const std::string& fileName);
	void ShowDList();
};
} // namespace atre
