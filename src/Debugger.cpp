#include <iostream>
#include <bitset>
#include "Debugger.hpp"
#include "Atari.hpp"
#include "Chips.hpp"
#include "ANTIC.hpp"

using namespace std;

namespace atre
{
Debugger::Debugger(Atari *atari) : mAtari(atari), mExiting(false), mStopping(false) {}

void Debugger::Initialize()
{
	mAtari->mCPU->Attach(this);
	mCPUThread = make_unique<thread>(std::bind(&atre::Debugger::CPUThread, this));
	mIOThread = make_unique<thread>(std::bind(&atre::Debugger::IOThread, this));
}

void Debugger::Exit()
{
	mExiting = true;
	mStopping = true;
	mRunning.notify_one();
	mCPUThread->join();
	mIOThread->join();
}

void Debugger::DumpCallStack()
{
	for (auto s : mAtari->mCPU->mCallStack)
	{
		cout << hex << s.first << " (" << s.second << ")" << endl;
	}
	cout << hex << mAtari->mCPU->PC << " (PC)" << endl;
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
	mAtari->mIO->KeyboardInput(command);
	/*	for (size_t i = 0; i < command.length(); i++)
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
	}*/
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
		}
		cout << "Stopping CPU execution" << endl;
	}
	//cout << "Exiting CPU thread" << endl;
}

void Debugger::IOThread()
{
	//cout << "Starting IO thread" << endl;
	mAtari->mIO->Initialize();
	auto tickInterval = chrono::duration<int, std::milli>(MILLISEC_PER_FRAME);
	auto pauseInterval = chrono::duration<int, std::milli>(1000);
	while (!mExiting)
	{
		//cout << "(IO Thread Step)" << endl;
		mAtari->mIO->Refresh(!mStopping);
		if (mStopping)
		{
			this_thread::sleep_for(pauseInterval);
		}
		else
		{
			this_thread::sleep_for(tickInterval);
		}
	}
	mAtari->mIO->Destroy();
	//cout << "Exiting IO thread" << endl;
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

void Debugger::DumpDList()
{
	word_t listStart = mAtari->mMemory->GetW(ChipRegisters::DLISTL);
	word_t listAddr = listStart;
	word_t lmsAddr = 0;
	do
	{
		byte_t ai = mAtari->mMemory->Get(listAddr);
		byte_t mode = ai & 0b1111;
		bool DLI = ai & 0b10000000;
		bool LMS = ai & 0b01000000;
		bool VS = ai & 0b00100000;
		bool HS = ai & 0b00010000;
		switch (mode)
		{
		case 0:
		{
			auto numBlanks = ((ai >> 4) & 0b111) + 1;
			cout << "Blank x" << dec << numBlanks;
			if (DLI)
			{
				cout << " (DLI)";
			}
			cout << endl;
			listAddr++;
			break;
		}
		case 0x01:
		case 0x41:
		{
			auto jumpAddr = mAtari->mMemory->GetW(listAddr + 1);
			cout << (LMS ? "JVB" : "JMP") << " " << hex << jumpAddr << dec << endl;
			listAddr = jumpAddr;
			break;
		}
		default:
		{
			if (LMS)
			{
				lmsAddr = mAtari->mMemory->GetW(listAddr + 1);
			}
			if (mode < 8)
			{
				if (mode == 2)
				{
					auto pf = mAtari->mMemory->Get(ChipRegisters::DMACTL) & 0b11;
					int numChars = 40;
					if (pf == 1)
					{
						numChars = 32;
					}
					else if (pf == 3)
					{
						numChars = 48;
					}
					for (int cn = 0; cn < numChars; cn++)
					{
						byte_t bc = mAtari->mMemory->Get(lmsAddr);
						char c = static_cast<char>((bc & 0x7F) + 0x20);
						if (isalnum(c))
						{
							cout << c;
						}
						else
						{
							cout << ".";
						}
						lmsAddr++;
					}
					cout << " Mode 2";
				}
				else
				{
					cout << "Mode " << hex << (int)mode << dec << " (Char)";
				}
			}
			else
			{
				cout << "Mode " << hex << (int)mode << dec << " (Map)";
			}
			if (HS)
			{
				cout << " (HS)";
			}
			if (VS)
			{
				cout << " (VS)";
			}
			if (LMS)
			{
				cout << " (LMS " << hex << lmsAddr << dec << ")";
				listAddr += 2;
			}
			if (DLI)
			{
				cout << " (DLI)";
			}
			cout << endl;

			listAddr++;
		}
		}
	} while (listAddr != listStart);
}

void Debugger::BASIC()
{
	mAtari->mCPU->JumpTo(0xA000);
}

} // namespace atre
