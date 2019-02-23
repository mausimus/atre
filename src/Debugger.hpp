#pragma once

#include "CPU.hpp"

namespace atre
{
class Debugger
{
  public:
	static void DumpCPU(const CPU &cpu);
};
} // namespace atre
