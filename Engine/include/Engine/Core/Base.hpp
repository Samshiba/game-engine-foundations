//
// Created by genin on 27/01/2026.
// Path: Engine/include/Engine/Core/Base.hpp
//

#pragma once

// Force Inline
#if defined(_MSC_VER)
#define GEF_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define GEF_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define GEF_FORCE_INLINE inline
#endif

// Debug Break
#if defined(_MSC_VER)
#define GEF_DEBUGBREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#if __has_builtin(__builtin_trap)
#define GEF_DEBUGBREAK() __builtin_trap()
#else
#define GEF_DEBUGBREAK() __asm__ volatile("int $0x03") // Pour x86/x64
#endif
#else
#include <signal.h>
#define GEF_DEBUGBREAK() raise(SIGTRAP)
#endif