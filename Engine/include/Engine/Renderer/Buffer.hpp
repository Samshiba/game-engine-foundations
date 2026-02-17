//
// Created by genin on 08/02/2026.
// Path: Engine/include/Engine/Renderer/Buffer.hpp
//

#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <string>

#include "Engine/Core/Log.hpp"
#include "Engine/Core/Base.hpp"

namespace GEF::Renderer
{
    enum class ShaderDataType : uint32_t
    {
        None = 0,
        Float,
        Float2,
        Float3,
        Float4,
        Mat3,
        Mat4,
        Int,
        Int2,
        Int3,
        Int4,
        Bool
    };

    struct BufferElement
    {
        std::string name;
        ShaderDataType type;
        uint32_t size;
        size_t offset;
        bool normalized;

        BufferElement(ShaderDataType type, std::string name,
                      bool normalized = false);

        [[nodiscard]] static uint32_t GetSizeOfType(ShaderDataType type);
        [[nodiscard]] uint8_t GetComponentCount() const;
    };

    class BufferLayout
    {
    public:
        BufferLayout() = default;
        BufferLayout(const std::initializer_list<BufferElement>& elements);

        [[nodiscard]] const std::vector<BufferElement>& GetElements() const;
        [[nodiscard]] uint32_t GetStride() const;

        std::vector<BufferElement>::iterator begin();
        std::vector<BufferElement>::iterator end();
        [[nodiscard]] std::vector<BufferElement>::const_iterator begin() const;
        [[nodiscard]] std::vector<BufferElement>::const_iterator end() const;

    private:
        void CalculateOffsetsAndStride();

    private:
        std::vector<BufferElement> elements_;
        uint32_t stride_ = 0;
    };

    inline BufferElement::BufferElement(ShaderDataType type, std::string name,
                                        bool normalized)
        : name(std::move(name)), type(type), size(GetSizeOfType(type)),
          offset(0),
          normalized(normalized)
    {
    }

    inline uint32_t BufferElement::GetSizeOfType(ShaderDataType type)
    {
        switch (type)
        {
            using enum ShaderDataType;
        case Float:
            return 4;
        case Float2:
            return 4 * 2;
        case Float3:
            return 4 * 3;
        case Float4:
            return 4 * 4;
        case Mat3:
            return 4 * 3 * 3;
        case Mat4:
            return 4 * 4 * 4;
        case Int:
            return 4;
        case Int2:
            return 4 * 2;
        case Int3:
            return 4 * 3;
        case Int4:
            return 4 * 4;
        case Bool:
            return 1;
        default:
            GEF_ENGINE_ERROR("Unknown ShaderDataType");
            return 0;
        }
    }

    inline uint8_t BufferElement::GetComponentCount() const
    {
        switch (type)
        {
            using enum ShaderDataType;
        case Float:
            return 1;
        case Float2:
            return 2;
        case Float3:
            return 3;
        case Float4:
            return 4;
        case Mat3:
            return 3;
        case Mat4:
            return 4;
        case Int:
            return 1;
        case Int2:
            return 2;
        case Int3:
            return 3;
        case Int4:
            return 4;
        case Bool:
            return 1;
        default:
            //TODO ASSERT
            GEF_ENGINE_ERROR("Unknown ShaderDataType");
            return 0;
        }
    }

    inline BufferLayout::BufferLayout(
        const std::initializer_list<BufferElement>& elements)
        : elements_(elements)
    {
        CalculateOffsetsAndStride();
    }

    GEF_FORCE_INLINE const std::vector<BufferElement>&
    BufferLayout::GetElements() const
    {
        return elements_;
    }

    GEF_FORCE_INLINE uint32_t BufferLayout::GetStride() const
    {
        return stride_;
    }

    inline std::vector<BufferElement>::iterator BufferLayout::begin()
    {
        return elements_.begin();
    }

    inline std::vector<BufferElement>::iterator BufferLayout::end()
    {
        return elements_.end();
    }

    inline std::vector<BufferElement>::const_iterator
    BufferLayout::begin() const
    {
        return elements_.begin();
    }

    inline std::vector<BufferElement>::const_iterator BufferLayout::end() const
    {
        return elements_.end();
    }

    inline void BufferLayout::CalculateOffsetsAndStride()
    {
        size_t offset = 0;
        stride_ = 0;
        for (auto& element : elements_)
        {
            element.offset = offset;
            offset += element.size;
            stride_ += element.size;
        }
    }
}