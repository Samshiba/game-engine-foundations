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
#elif defined(__linux__)
#include <signal.h>
#define GEF_DEBUGBREAK() raise(SIGTRAP)
#else
#define GEF_DEBUGBREAK()
#endif