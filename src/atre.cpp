#include <iostream>
#include "Atari.hpp"
#include "Tests.hpp"
#include "Debugger.hpp"
#include "Chips.hpp"
#include "ANTIC.hpp"

//#include <SDL.h>

using namespace atre;
using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{
	cout << "atre Atari emulator" << endl;

	Atari atari;
	Debugger debugger(&atari);
	debugger.Initialize();

	bool isExiting = false;
	do
	{
		string command;
		cout << ">" << flush;
		getline(cin, command);

		if (command == "exit")
		{
			debugger.Exit();
			isExiting = true;
		}
		else if (command.empty())
		{
			continue;
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
		else if (command == "ehbasic")
		{
			Tests::EhBASIC(atari, debugger.mKeyboardInput, debugger.mScreenOutput);
		}
		// prepend with > to send text to input buffer
		else if (command[0] == '>')
		{
			// prepend with >> to send newline at the end
			if (command.length() > 1 && command[1] == '>')
			{
				debugger.Input(command.substr(2));
				debugger.Input("\r");
				debugger.Input("\n");
			}
			else
			{
				debugger.Input(command.substr(1));
			}
		}
		else if (command == "program")
		{
			const string program{"10 FOR I = 1 TO 10\r\n20 PRINT I\r\n30 NEXT I\r\nRUN\r\n"};
			debugger.Input(program);
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
			Tests::Boot(atari);
			debugger.Start();
		}
		else if (command == "dumpreg")
		{
			debugger.DumpReg("d000-d800.bin");
		}
		else if (command == "dumpmem")
		{
			debugger.DumpMem("atre-mem.bin");
		}
		else if (command == "steps on")
		{
			debugger.Steps(true);
		}
		else if (command == "selftest")
		{
			Tests::SelfTest(atari);
		}
		else if (command == "steps off")
		{
			debugger.Steps(false);
		}
		else if (command == "vblank int")
		{
			debugger.VBlank();
		}
		else if (command == "dlist int")
		{
			debugger.DList();
		}
		else if (command == "dumpdlist")
		{
			debugger.DumpDList();
		}
		else if (command == "callstack")
		{
			debugger.DumpCallStack();
		}
		else if (command == "basic")
		{
			debugger.BASIC();
		}
		else if (command == "serint1")
		{
			debugger.SerInt1();
		}
		else if (command == "serint2")
		{
			debugger.SerInt2();
		}
		else if (command == "dump")
		{
			debugger.Dump();
		}
	} while (!isExiting);
}
