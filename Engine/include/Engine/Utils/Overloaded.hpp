//
// Created by genin on 30/10/2026.
// Path: Engine/include/Engine/Utils/Overloaded.hpp
//

#pragma once

// helper type for variant visit

namespace GEF::Utils
{
    template <class... Ts>
    struct Overloaded : Ts...
    {
        using Ts::operator()...;
    };
}