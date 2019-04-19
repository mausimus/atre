#pragma once

#include "CPU.hpp"
#include "IO.hpp"
#include "RAM.hpp"

namespace atre
{
class Atari
{
public:
    Atari();

    inline CPU* getCPU()
    {
        return m_CPU.get();
    }

    inline IO* getIO()
    {
        return m_IO.get();
    }

    inline RAM* getRAM()
    {
        return m_RAM.get();
    }

    void Reset();
    void Boot(const std::string& osROM, const std::string& carridgeROM);

private:
    std::unique_ptr<RAM> m_RAM;
    std::unique_ptr<CPU> m_CPU;
    std::unique_ptr<IO>  m_IO;
};
} // namespace atre
