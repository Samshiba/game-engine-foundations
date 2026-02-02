//
// Created by genin on 02/02/2026.
//

#pragma once

namespace GEF
{
    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;
    };
}