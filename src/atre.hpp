#pragma once

#include <iostream>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <tuple>
#include <map>
#include <memory>
#include <deque>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <functional>
#include <fstream>

typedef uint8_t byte_t;
typedef int8_t sbyte_t;
typedef uint16_t word_t;
typedef int16_t sword_t;
typedef byte_t flag_t;

constexpr int MEM_SIZE = 65536;
constexpr int CYCLES_PER_SEC = 1790000;
