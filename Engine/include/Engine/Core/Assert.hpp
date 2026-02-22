//
// Created by genin on 21/02/2026.
// Path: Engine/include/Engine/Core/Assert.hpp
//

#pragma once

#if defined(GEF_ENABLE_ASSERTS)
#include "Base.hpp"
#include "Log.hpp"

#include <filesystem>
#endif

#ifdef GEF_ENABLE_ASSERTS

#define GEF_CORE_ASSERT(condition, ...) \
    do \
    { \
        if (!(condition)) [[unlikely]] \
        { \
            GEF_ENGINE_CRITICAL("Assertion '{0}' failed at {1}:{2}", #condition, std::filesystem::path(__FILE__).filename().string(), __LINE__); \
            __VA_OPT__(GEF_ENGINE_CRITICAL(__VA_ARGS__);) \
            GEF_DEBUGBREAK(); \
        } \
    } while (false)

#define GEF_ASSERT(condition, ...) \
    do \
    { \
        if (!(condition)) [[unlikely]] \
        { \
            GEF_CRITICAL("Assertion '{0}' failed at {1}:{2}", #condition, std::filesystem::path(__FILE__).filename().string(), __LINE__); \
            __VA_OPT__(GEF_CRITICAL(__VA_ARGS__);) \
            GEF_DEBUGBREAK(); \
        } \
    } while (false)

#else

#define GEF_CORE_ASSERT(condition, ...) do { (void)sizeof(condition); } while(false)
#define GEF_ASSERT(condition, ...)      do { (void)sizeof(condition); } while(false)

#endif