//
// Created by genin on 11/11/2026.
// Path: Engine/include/Engine/Utils/HashCombine.hpp
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace GEF::Utils
{
    template <typename T>
    void HashCombine(std::size_t& seed, const T& value)
    {
        std::size_t h = std::hash<T>{}(value);
        // 64-bit golden ratio constant + bit mixing
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        seed ^= h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    template <typename... Types>
    std::size_t HashAll(const Types&... args)
    {
        std::size_t seed = 0;
        (HashCombine(seed, args), ...);
        return seed;
    }
} // namespace GEF::Utils