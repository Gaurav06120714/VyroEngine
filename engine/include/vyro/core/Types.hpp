// VyroEngine — Core Types
// Phase 1.1: fundamental fixed-width type aliases and engine-wide macros.
#pragma once

#include <cstddef>
#include <cstdint>

namespace vyro {

// Fixed-width integer aliases.
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;
using byte  = std::byte;

// Compiler / platform detection.
#if defined(_WIN32)
    #define VYRO_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define VYRO_PLATFORM_MACOS 1
#elif defined(__linux__)
    #define VYRO_PLATFORM_LINUX 1
#endif

#if defined(_MSC_VER)
    #define VYRO_DEBUG_BREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
    #define VYRO_DEBUG_BREAK() __builtin_trap()
#else
    #define VYRO_DEBUG_BREAK() ((void)0)
#endif

// Non-copyable mix-in base for engine systems.
class NonCopyable {
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

} // namespace vyro
