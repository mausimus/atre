#pragma once

#include <SDL.h>
#include "Chips.hpp"
#include "ANTIC.hpp"

namespace atre
{
	class IOBase
	{
	protected:
		CPU *_CPU;
		Memory *_Memory;

	public:
		IOBase(CPU *cpu, Memory *memory) : _CPU(cpu), _Memory(memory) {}

		virtual void Initialize() {}
		virtual void Refresh(bool) {}
		virtual void Destroy() {}

		virtual void Tick() {}
		virtual void Reset() {}
	};

	class IO : public IOBase
	{
		ANTIC _ANTIC;
		GTIA _GTIA;
		POKEY _POKEY;
		PIA _PIA;

		static std::map<SDL_Scancode, byte_t> _scanCodes;

		SDL_Window *_window;
		SDL_Renderer *_renderer;
		SDL_Texture *_texture;

	public:
		IO(CPU *cpu, Memory *memory);

		void Initialize() override;
		void Refresh(bool cpuRunning) override;
		void Destroy() override;

		void Tick() override;
		void Reset() override;
		void Write(word_t reg, byte_t val);
		byte_t Read(word_t reg);
	};
} // namespace atre