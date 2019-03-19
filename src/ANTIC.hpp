#pragma once

#include "Chips.hpp"

namespace atre
{
class ANTIC : public Chip
{
	word_t _lineNum;
	word_t _lineCycle;

	uint32_t _frameBuffer[FRAME_HEIGHT * FRAME_WIDTH];

	int _renderLine;
	byte_t _mode;
	word_t _width;
	word_t _lmsAddr;
	word_t _listAddr;
	bool _triggerDLI;
	bool _listActive;
	std::chrono::time_point<std::chrono::steady_clock> _lastFrameTime;

	static uint32_t _palette[128];

	uint32_t GetRGB(byte_t color) const;

	void StartDisplayList();
	void StepDisplayList();

	void Blank(int numBlanks);
	void CharacterLine();
	void MapLine();
	void Fill(uint32_t *lineStart, uint32_t color, int width);

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
