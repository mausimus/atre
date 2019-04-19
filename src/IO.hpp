#pragma once

#include "ANTIC.hpp"
#include "Chips.hpp"
#include <SDL.h>

namespace atre
{
class IO
{
public:
    IO(CPU* cpu, RAM* ram);

    void Initialize();
    void Refresh(bool cpuRunning);
    void Destroy();

    void   Tick();
    void   Reset();
    void   Write(word_t reg, byte_t val);
    byte_t Read(word_t reg);

private:
    const static std::map<SDL_Scancode, int> s_scanCodes;

    CPU* m_CPU;

    ANTIC m_ANTIC;
    GTIA  m_GTIA;
    POKEY m_POKEY;
    PIA   m_PIA;

    SDL_Window*   m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture*  m_texture;
};
} // namespace atre
