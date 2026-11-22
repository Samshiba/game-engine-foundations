//
// Created by genin on 06/11/2026.
// Path: Engine/include/Engine/Core/Embedded.hpp
//

#pragma once

#include <string_view>

namespace GEF::Embedded
{
    std::string_view LitVertexSource();
    std::string_view LitFragmentSource();
    std::string_view ShadowVertexSource();
    std::string_view ShadowFragmentSource();
} // namespace GEF::Embedded