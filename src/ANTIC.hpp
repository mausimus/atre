#pragma once

#include "Chips.hpp"

namespace atre
{
using time_point = std::chrono::time_point<std::chrono::steady_clock>;

class ANTIC : public Chip
{
public:
    ANTIC(CPU* cpu, RAM* ram);
    ~ANTIC();

    static uint32_t getColor(byte_t color);
    const uint32_t* getDisplayBuffer() const;
    ScanBuffer*     getScanBuffer();

    void   Reset() override;
    void   Tick() override;
    void   Write(word_t reg, byte_t val) override;
    byte_t Read(word_t reg) override;

private:
    static const uint32_t s_palette[128];

    uint32_t   m_renderBuffer[FRAME_HEIGHT * FRAME_WIDTH];
    uint32_t   m_displayBuffer[FRAME_HEIGHT * FRAME_WIDTH];
    int        m_renderLine;
    byte_t     m_displayMode;
    int        m_playfieldWidth;
    word_t     m_scanAddress;
    word_t     m_listAddress;
    bool       m_triggerDLI;
    bool       m_hScroll;
    bool       m_listActive;
    time_point m_lastFrameTime;
    word_t     m_scanLine;
    ScanBuffer m_scanBuffer;

    void StartDisplayList();
    void StepDisplayList();
    void RenderBlankLines(int numBlanks);
    void RenderCharacterLine();
    void RenderMapLine();
    void Fill(uint32_t* lineStart, uint32_t color, int width);
};
} // namespace atre
