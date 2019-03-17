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
	uint32_t _screenBuffer[SCREEN_HEIGHT * SCREEN_WIDTH];
	int _y;
	byte_t _mode;
	word_t _width;
	word_t _lmsAddr;

	static uint32_t _palette[128];

	uint32_t GetRGB(byte_t color) const;
	std::unordered_set<int> _DLIs;

	void RenderDisplayList();
	void DrawFrame();
	void Blank(int numBlanks);
	void CharacterLine();
	void MapLine();
	void Fill(uint32_t *lineStart, uint32_t color, int width);

  public:
	ANTIC(CPU *cpu, Memory *memory);
	~ANTIC();

	uint32_t *GetScreenBuffer() { return _screenBuffer; }
	void Reset() override;
	void Tick() override;
	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
};
} // namespace atre
