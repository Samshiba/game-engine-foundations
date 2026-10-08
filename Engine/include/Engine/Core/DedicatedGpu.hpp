//
// Created by genin on 08/10/2026.
// Path: Engine/include/Engine/Core/DedicatedGpu.hpp
//

#pragma once

#if defined(_WIN32)
#define GEF_REQUEST_DEDICATED_GPU()                                            \
    extern "C"                                                                 \
    {                                                                          \
        __declspec(dllexport) unsigned long NvOptimusEnablement = 1;           \
        __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;    \
    }                                                                          \
    static_assert(true, "") /* lets the call site end with a semicolon */
#else
#define GEF_REQUEST_DEDICATED_GPU() static_assert(true, "")
#endif
