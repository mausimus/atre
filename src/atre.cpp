#include <iostream>
#include "Atari.hpp"
#include "Tests.hpp"
#include "Debugger.hpp"
#include "Chips.hpp"
#include "ANTIC.hpp"

using namespace atre;
using namespace std;

int main(int /*argc*/, char * /*argv*/[])
{
	cout << "atre Atari emulator" << endl;

	Atari atari;
	Debugger debugger(&atari);
	debugger.Initialize();

	bool isExiting = false;
	do
	{
		string line;
		cout << ">" << flush;
		getline(cin, line);
		if (!line.length())
		{
			continue;
		}
		istringstream commands(line);
		string command;
		commands >> command;

		if (command.empty())
		{
			continue;
		}
		else if (command == "exit")
		{
			debugger.Exit();
			isExiting = true;
		}
		else if (command == "help")
		{
			cout << "Type `boot <os_rom_file> [cartridge_rom_file]` to start the emulator" << endl;
			cout << "<os_rom_file> should be the Atari XL OS ROM image (16 kB, Rev B)" << endl;
			cout << "[cartridge_rom_file] is an optional additional 8kB ROM, either BASIC or cartridge" << endl;
			cout << "If no cartridge ROM is loaded the system will start Self Test" << endl;
			cout << "Use `stop` and `start` to pause the CPU and `exit` to quit" << endl;
			cout << "[F1] = Help, [F2] = Start, [F3] = Select, [F4] = Option, [F5] = Reset, [F6] = Break" << endl;
			cout << "[Arrow Keys] = Joystick, [Left Ctrl] = Fire" << endl;
		}
		else if (command == "test1")
		{
			Tests::FunctionalTest(atari);
		}
		else if (command == "test2")
		{
			Tests::InterruptTest(atari);
		}
		else if (command == "test3")
		{
			Tests::AllSuiteA(atari);
		}
		else if (command == "test4")
		{
			Tests::TimingTest(atari);
		}
		else if (command == "start")
		{
			debugger.Start();
		}
		else if (command == "stop")
		{
			debugger.Stop();
		}
		else if (command == "boot")
		{
			if (commands.eof())
			{
				cout << "Please specify OS ROM and optionally cartridge ROM file names." << endl;
				continue;
			}
			string osROM;
			string cartridgeROM;
			commands >> osROM;
			if (!commands.eof())
			{
				commands >> cartridgeROM;
			}
			Tests::Boot(atari, osROM, cartridgeROM);
			debugger.Start();
		}
		else if (command == "dumpmem")
		{
			debugger.DumpMem("atre-mem.bin");
		}
		else if (command == "steps on")
		{
			debugger.Steps(true);
		}
		else if (command == "steps off")
		{
			debugger.Steps(false);
		}
		else if (command == "dumpdlist")
		{
			debugger.DumpDList();
		}
		else if (command == "callstack")
		{
			debugger.DumpCallStack();
		}
		else if (command == "dumpcpu")
		{
			debugger.Dump();
		}
	} while (!isExiting);

	return 0;
}