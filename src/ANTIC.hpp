#pragma once

#include "Chips.hpp"

namespace atre
{
class ANTIC : public Chip
{
	word_t _lineNum;
	word_t _lineCycle;

	// rendering state
	uint32_t _frameBuffer[SCREEN_HEIGHT * SCREEN_WIDTH];
	byte_t _y;
	byte_t _mode;
	word_t _width;
	word_t _lmsAddr;

	uint32_t GetRGB(byte_t color) const;

	void RenderDisplayList();
	void DrawFrame();
	void Blank(int numBlanks);
	void CharacterLine();

  public:
	ANTIC(CPU *cpu, Memory *memory);
	~ANTIC();

	uint32_t *GetFrameBuffer() { return _frameBuffer; }
	void Reset() override;
	void Tick() override;
	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
};
} // namespace atre
