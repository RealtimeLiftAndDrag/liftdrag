#pragma once

#include <memory>
#include <string>

#include "glm/glm.hpp"



using glm:: vec2, glm:: vec3, glm:: vec4;
using glm::ivec2, glm::ivec3, glm::ivec4;
using glm::uvec2, glm::uvec3, glm::uvec4;
using glm::bvec2, glm::bvec3, glm::bvec4;
using glm::dvec2, glm::dvec3, glm::dvec4;

using glm::mat2, glm::mat3, glm::mat4;

using glm::quat;

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

namespace detail {
    template <typename T> struct FPtr;
    template <typename Ret, typename... Args>
    struct FPtr<Ret(Args...)> {
        using Type = Ret (*)(Args...);
    };
}
template <typename T> using fptr = typename detail::FPtr<T>::Type;

template <typename T> using unq = std::unique_ptr<T>;
template <typename T> using shr = std::shared_ptr<T>;

template <typename T1, typename T2> using pair = std::pair<T1, T2>;
template <typename T> using duo = pair<T, T>;

using std::move;

using namespace std::string_literals;

template <typename T> constexpr T k_infinity{std::numeric_limits<T>::infinity()};
template <typename T> constexpr T k_nan{std::numeric_limits<T>::quiet_nan()};

extern std::string g_resourcesDir;
