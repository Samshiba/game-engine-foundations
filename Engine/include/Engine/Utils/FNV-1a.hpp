//
// Created by genin on 31/01/2026.
//

#pragma once

#include <cstdint>

namespace GEF::Utils
{
    // FNV-1a 32-bit constants
    constexpr std::uint32_t FNV_PRIME = 16777619u;
    constexpr std::uint32_t FNV_OFFSET_BASIS = 2166136261u;

    constexpr std::uint32_t HashFNV1a(const char* str)
    {
        std::uint32_t hash = FNV_OFFSET_BASIS;
        while (*str)
        {
            hash ^= static_cast<std::uint32_t>(*str);
            hash *= FNV_PRIME;
            ++str;
        }

        return hash;
    }

    constexpr std::uint32_t operator""_hash(const char* s, std::size_t)
    {
        return HashFNV1a(s);
    }
}