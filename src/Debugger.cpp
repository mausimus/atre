#include "ANTIC.hpp"
#include "Atari.hpp"
#include "Chips.hpp"
#include "Debugger.hpp"
#include <bitset>
#include <iostream>

using namespace std;

namespace atre
{
Debugger::Debugger(Atari* atari) :
	mAtari(atari), m_exiting(), m_stopping(), m_mutex(), m_running(), m_CPUThread(), m_IOThread()
{}

void Debugger::Initialize()
{
	mAtari->CPU()->Attach(this);
	m_CPUThread = make_unique<thread>(bind(&atre::Debugger::CPUThread, this));
	m_IOThread	= make_unique<thread>(bind(&atre::Debugger::IOThread, this));
}

void Debugger::Exit()
{
	m_exiting  = true;
	m_stopping = true;
	m_running.notify_one();
	m_CPUThread->join();
	m_IOThread->join();
}

void Debugger::CallStack()
{
	for(auto s : mAtari->CPU()->m_callStack)
	{
		cout << hex << s.first << " (" << s.second << ")" << endl;
	}
	cout << hex << mAtari->CPU()->PC << " (PC)" << endl;
}

void Debugger::DumpState()
{
	const CPU& cpu = *(mAtari->CPU());
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
	m_stopping = false;
	m_running.notify_one();
}

void Debugger::Stop()
{
	m_stopping = true;
}

void Debugger::CPUThread()
{
	// cout << "Starting CPU thread" << endl;
	unique_lock<mutex> runningLock(m_mutex);
	while(!m_exiting)
	{
		// cout << "(CPU Thread Step)" << endl;
		m_running.wait(runningLock);
		if(m_exiting)
		{
			break;
		}
		cout << "Starting CPU execution" << endl;
		while(!m_stopping)
		{
			mAtari->CPU()->Execute();
		}
		cout << "Stopping CPU execution" << endl;
	}
	// cout << "Exiting CPU thread" << endl;
}

void Debugger::IOThread()
{
	// cout << "Starting IO thread" << endl;
	mAtari->IO()->Initialize();
	auto tickInterval  = chrono::duration<int, std::milli>(MILLISEC_PER_FRAME);
	auto pauseInterval = chrono::duration<int, std::milli>(1000);
	while(!m_exiting)
	{
		// cout << "(IO Thread Step)" << endl;
		mAtari->IO()->Refresh(!m_stopping);
		if(m_stopping)
		{
			this_thread::sleep_for(pauseInterval);
		}
		else
		{
			this_thread::sleep_for(tickInterval);
		}
	}
	mAtari->IO()->Destroy();
	// cout << "Exiting IO thread" << endl;
}

void Debugger::Steps(bool steps)
{
	mAtari->CPU()->m_showSteps = steps;
}

void Debugger::DumpRAM(const string& fileName)
{
	std::ofstream ofs(fileName, std::ios_base::binary);

	if(!ofs.good())
	{
		throw std::runtime_error("Unable to open file");
	}

	byte_t visibleMem[MEM_SIZE];
	for(word_t i = 0; i < MEM_SIZE; i++)
	{
		visibleMem[i] = mAtari->RAM()->Get(i);
	}

	ofs.write(reinterpret_cast<char*>(visibleMem), 0x10000);

	ofs.close();
}

void Debugger::ShowDList()
{
	word_t listStart = mAtari->RAM()->GetW(ChipRegisters::DLISTL);
	word_t listAddr	 = listStart;
	word_t lmsAddr	 = 0;
	do
	{
		byte_t ai	= mAtari->RAM()->Get(listAddr);
		byte_t mode = ai & 0b1111;
		bool   DLI	= ai & 0b10000000;
		bool   LMS	= ai & 0b01000000;
		bool   VS	= ai & 0b00100000;
		bool   HS	= ai & 0b00010000;
		switch(mode)
		{
		case 0:
		{
			auto numBlanks = ((ai >> 4) & 0b111) + 1;
			cout << "Blank x" << dec << numBlanks;
			if(DLI)
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
			auto jumpAddr = mAtari->RAM()->GetW(listAddr + 1);
			cout << (LMS ? "JVB" : "JMP") << " " << hex << jumpAddr << dec << endl;
			listAddr = jumpAddr;
			break;
		}
		default:
		{
			if(LMS)
			{
				lmsAddr = mAtari->RAM()->GetW(listAddr + 1);
			}
			if(mode < 8)
			{
				if(mode == 2)
				{
					auto pf		  = mAtari->RAM()->Get(ChipRegisters::DMACTL) & 0b11;
					int	 numChars = 40;
					if(pf == 1)
					{
						numChars = 32;
					}
					else if(pf == 3)
					{
						numChars = 48;
					}
					for(int cn = 0; cn < numChars; cn++)
					{
						byte_t bc = mAtari->RAM()->Get(lmsAddr);
						char   c  = static_cast<char>((bc & 0x7F) + 0x20);
						if(isalnum(c))
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
			if(HS)
			{
				cout << " (HS)";
			}
			if(VS)
			{
				cout << " (VS)";
			}
			if(LMS)
			{
				cout << " (LMS " << hex << lmsAddr << dec << ")";
				listAddr += 2;
			}
			if(DLI)
			{
				cout << " (DLI)";
			}
			cout << endl;

			listAddr++;
		}
		}
	} while(listAddr != listStart);
}
} // namespace atre
