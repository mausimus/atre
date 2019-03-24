## atre

Simple 8-bit Atari platform (XL/XE) emulator in C++

### Features

* Full MOS 6502 CPU emulation
* Graphics/IO hardware registers emulation
* boots original Atari XL OS
* runs Atari BASIC
* passes Self Test
* runs a classic game

Only the necessary features to run the above have been implemented,
the project doesn't aspire to compete with any well-established emulators!

Feel free to try running your favourite game and enhancing the emulator to
do so correctly.

### Screenshots

###### Original BASIC

![screenshot](images/screen1.png)

###### Self-Test

![screenshot](images/screen2.png)

###### Classic Game

![screenshot](images/screen4.png)

###### Debugger

![screenshot](images/screen3.png)

### Code

Cross-platform C++ 17, reference build using gcc 7.3 and make 4.1 on Ubuntu 18.04
and SDL 2.0.8 for graphics and IO.

In order to start the emulator you will need an original Atari OS ROM and
either the BASIC ROM or a custom game ROM (cartridge). Type _help_ in
the debugger window for more info. No ROMs are included in this repository!

### Thanks

Big thanks in particular to:

* Klaus for his 6502 functional tests: https://github.com/Klaus2m5/6502_65C02_functional_tests

* Avery for his Altirra emulator and fantastic documentation: http://www.virtualdub.org/altirra.html

* everyone else who keeps archives of 8-bit platform docs and sources running!
