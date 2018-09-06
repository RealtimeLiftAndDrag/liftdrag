#pragma once



#include <memory>

#include "glm/glm.hpp"



using glm::vec2;
using glm::vec3;
using glm::vec4;

using glm::ivec2;
using glm::ivec3;
using glm::ivec4;

using glm::bvec2;
using glm::bvec3;
using glm::bvec4;

using glm::mat2;
using glm::mat3;
using glm::mat4;

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using llong = long long;
using ullong = unsigned long long;

using s08 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using u08 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

template <typename T> using unq = std::unique_ptr<T>;
template <typename T> using shr = std::shared_ptr<T>;