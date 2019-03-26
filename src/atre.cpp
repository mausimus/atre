#include "ANTIC.hpp"
#include "Atari.hpp"
#include "Chips.hpp"
#include "Debugger.hpp"
#include "Tests.hpp"
#include "atre.hpp"

using namespace atre;
using namespace std;

int main(int /*argc*/, char* /*argv*/[])
{
	cout << "== atre Atari emulator ==" << endl;
	cout << "Type help for command list" << endl;

	Atari	 atari;
	Debugger debugger(&atari);
	debugger.Initialize();

	bool isExiting = false;
	do
	{
		try
		{
			string line;
			cout << ">" << flush;
			getline(cin, line);
			if(!line.length())
			{
				continue;
			}
			istringstream commands(line);
			string		  command;
			commands >> command;

			if(command.empty())
			{
				continue;
			}
			else if(command == "exit")
			{
				debugger.Exit();
				isExiting = true;
			}
			else if(command == "help")
			{
				cout << "Commands:" << endl;
				cout << "- boot <os_rom_file> [cartridge_rom_file]: start the emulator" << endl;
				cout << "  <os_rom_file> should be the Atari XL OS ROM image (16 kB, Rev B)" << endl;
				cout << "  [cartridge_rom_file] is an optional additional 8kB ROM (BASIC or another cartridge)" << endl;
				cout << "- tests: run internal testing suites" << endl;
				cout << "- start and stop: control CPU execution" << endl;
				cout << "- exit" << endl;
			}
			else if(command == "tests")
			{
				cout << "Running test suites..." << endl;
				Tests::FunctionalTest();
				Tests::InterruptTest();
				Tests::AllSuiteA();
				Tests::TimingTest();
			}
			else if(command == "start")
			{
				debugger.Start();
			}
			else if(command == "stop")
			{
				debugger.Stop();
			}
			else if(command == "boot")
			{
				if(commands.eof())
				{
					cout << "Please specify OS ROM and optionally cartridge ROM file names." << endl;
					continue;
				}
				string osROM;
				string cartridgeROM;
				commands >> osROM;
				if(!commands.eof())
				{
					commands >> cartridgeROM;
				}
				atari.Boot(osROM, cartridgeROM);
				cout << "[F1] = Help, [F2] = Start, [F3] = Select, [F4] = Option, [F5] = Reset, [F6] = Break" << endl;
				cout << "[Arrow Keys] = Joystick, [Left Ctrl] = Fire" << endl;
				debugger.Start();
			}
			else if(command == "dumpram")
			{
				debugger.DumpRAM("atre-mem.bin");
			}
			else if(command == "steps on")
			{
				debugger.Steps(true);
			}
			else if(command == "steps off")
			{
				debugger.Steps(false);
			}
			else if(command == "showdlist")
			{
				debugger.ShowDList();
			}
			else if(command == "callstack")
			{
				debugger.CallStack();
			}
			else if(command == "dumpstate")
			{
				debugger.DumpState();
			}
		}
		catch(exception& e)
		{
			cout << "Exception: " << e.what() << endl;
		}
	} while(!isExiting);

	return 0;
}
