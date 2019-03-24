#pragma once

#include "Chips.hpp"

namespace atre
{
class ANTIC : public Chip
{
	uint32_t _frameBuffer[FRAME_HEIGHT * FRAME_WIDTH];
	uint32_t _screenBuffer[FRAME_HEIGHT * FRAME_WIDTH];

	int _renderLine;
	byte_t _mode;
	word_t _width;
	word_t _lmsAddr;
	word_t _listAddr;
	bool _triggerDLI;
	bool _hs;
	bool _listActive;
	std::chrono::time_point<std::chrono::steady_clock> _lastFrameTime;

	static uint32_t _palette[128];

	void StartDisplayList();
	void StepDisplayList();

	void Blank(int numBlanks);
	void CharacterLine();
	void MapLine();
	void Fill(uint32_t *lineStart, uint32_t color, int width);

  public:
	ANTIC(CPU *cpu, Memory *memory);
	~ANTIC();

	word_t _lineNum;
	word_t _lineCycle;
	byte_t _linePlayfield[FRAME_WIDTH];

	uint32_t GetRGB(byte_t color) const;
	uint32_t *GetFrameBuffer() { return _frameBuffer; }
	uint32_t *GetScreenBuffer() { return _screenBuffer; }
	void Reset() override;
	void Tick() override;
	void Write(word_t reg, byte_t val) override;
	byte_t Read(word_t reg) override;
};
} // namespace atre
