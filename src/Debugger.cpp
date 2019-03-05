#include <iostream>
#include <bitset>
#include "Debugger.hpp"
#include "Atari.hpp"

using namespace std;

namespace atre
{
Debugger::Debugger(Atari *atari) : mAtari(atari), mExiting(false), mStopping(false) {}

void Debugger::Initialize()
{
	mCPUThread = make_unique<thread>(std::bind(&atre::Debugger::CPUThread, this));
	mKeyboardInput = make_shared<IOPort>();
	mScreenOutput = make_shared<IOPort>();
	mAtari->mCPU->Attach(this);
}

void Debugger::Exit()
{
	mExiting = true;
	mStopping = true;
	mRunning.notify_one();
	mCPUThread->join();
}

void Debugger::Dump()
{
	const CPU &cpu = *(mAtari->mCPU);
	cout << "A = " << hex << showbase << (int)cpu.A << " (" << dec << (int)cpu.A << "), ";
	cout << "X = " << hex << showbase << (int)cpu.X << " (" << dec << (int)cpu.X << "), ";
	cout << "Y = " << hex << showbase << (int)cpu.Y << " (" << dec << (int)cpu.Y << "), ";
	cout << "PC = " << hex << showbase << (int)cpu.PC << ", ";
	cout << "S = " << hex << showbase << (int)cpu.S << ", ";
	cout << "Flags = ";
	cout << (cpu.IsSetFlag(CPU::NEGATIVE_FLAG) ? "N" : "n");
	cout << (cpu.IsSetFlag(CPU::OVERFLOW_FLAG) ? "O" : "o");
	cout << (cpu.IsSetFlag(CPU::IGNORED_FLAG) ? "X" : "x");
	cout << (cpu.IsSetFlag(CPU::BREAK_FLAG) ? "B" : "b");
	cout << (cpu.IsSetFlag(CPU::DECIMAL_FLAG) ? "D" : "d");
	cout << (cpu.IsSetFlag(CPU::INTERRUPT_FLAG) ? "I" : "i");
	cout << (cpu.IsSetFlag(CPU::ZERO_FLAG) ? "Z" : "z");
	cout << (cpu.IsSetFlag(CPU::CARRY_FLAG) ? "C" : "c");
	cout << " " << bitset<8>(cpu.F) << endl;
}

// CPU callbacks
void Debugger::OnTrap()
{
	cout << "Trap" << endl;
	Stop();
}
void Debugger::OnIRQ()
{
	cout << "IRQ" << endl;
}
void Debugger::OnNMI()
{
	cout << "NMI" << endl;
}
void Debugger::OnBRK()
{
	cout << "BRK" << endl;
}
void Debugger::OnReset()
{
	cout << "Reset" << endl;
}
void Debugger::OnBreak()
{
	cout << "Break" << endl;
	Stop();
}

void Debugger::Start()
{
	mStopping = false;
	mRunning.notify_one();
}

void Debugger::Stop()
{
	mStopping = true;
}

void Debugger::Input(const string &command)
{
	for (size_t i = 0; i < command.length(); i++)
	{
		if (command[i] == '\\' && i < command.length() - 1)
		{
			switch (command[i + 1])
			{
			case 'r':
				mKeyboardInput->queue.push_back('\r');
				i++;
				break;
			case 'n':
				mKeyboardInput->queue.push_back('\n');
				i++;
				break;
			default:
				mKeyboardInput->queue.push_back(command[i]);
				break;
			}
		}
		else
		{
			mKeyboardInput->queue.push_back(command[i]);
		}
	}
}

void Debugger::CPUThread()
{
	//cout << "Starting CPU thread" << endl;
	unique_lock<mutex> runningLock(mRunningMutex);
	while (!mExiting)
	{
		//cout << "(CPU Thread Step)" << endl;
		mRunning.wait(runningLock);
		if (mExiting)
		{
			break;
		}
		cout << "Starting CPU execution" << endl;
		while (!mStopping)
		{
			mAtari->mCPU->Execute();

			while (!mScreenOutput->queue.empty())
			{
				cout << mScreenOutput->queue.front() << flush;
				mScreenOutput->queue.pop_front();
			}
		}
		cout << "Stopping CPU execution" << endl;
	}
	//cout << "Exiting CPU thread" << endl;
}

void Debugger::DumpReg(const string &fileName)
{
	std::ofstream ofs(fileName, std::ios_base::binary);

	if (!ofs.good())
	{
		throw std::runtime_error("Unable to open file");
	}

	ofs.write(reinterpret_cast<char *>(
				  mAtari->mMemory->_bytes) +
				  0xD000,
			  0x0800);

	ofs.close();
}

void Debugger::Steps(bool steps)
{
	mAtari->mCPU->mShowSteps = steps;
}

void Debugger::DumpMem(const string &fileName)
{
	std::ofstream ofs(fileName, std::ios_base::binary);

	if (!ofs.good())
	{
		throw std::runtime_error("Unable to open file");
	}

	byte_t visibleMem[65536];
	for (int i = 0; i < 65536; i++)
	{
		visibleMem[i] = mAtari->mMemory->Get(i);
	}

	ofs.write(reinterpret_cast<char *>(visibleMem),
			  0x10000);

	ofs.close();
}

void Debugger::VBlank()
{
	int i = 500;
	while (i-- > 0)
	{
		mAtari->mMemory->Set(0xD40F, 0b01011111);
		mAtari->mCPU->_nmiPending = true;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	cout << "Vblanks sent" << endl;
}

void Debugger::DList()
{
	mAtari->mMemory->Set(0xD40F, 0b10011111);
	mAtari->mCPU->_nmiPending = true;
}
} // namespace atre
