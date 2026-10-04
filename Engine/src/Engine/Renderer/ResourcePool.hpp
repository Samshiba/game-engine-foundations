//
// Created by genin on 04/10/2026.
// Path: Engine/src/Engine/Renderer/ResourcePool.hpp
//

#pragma once

#include <cstdint>
#include <vector>

#include <Engine/Renderer/Handle.hpp>
#include <Engine/Core/Assert.hpp>

namespace GEF::Renderer
{
    template <typename T, typename Tag>
    class ResourcePool
    {
    public:
        Handle<Tag> Insert(T value);

        [[nodiscard]] T* Get(Handle<Tag> h);
        [[nodiscard]] const T* Get(Handle<Tag> h) const;

        [[nodiscard]] bool IsValid(Handle<Tag> h) const;

        void Remove(Handle<Tag> h);

        [[nodiscard]] std::vector<Handle<Tag> > GetAliveHandles() const;

    private:
        struct Slot
        {
            T value;
            uint32_t generation = 0;
            bool alive = false;
        };

        std::vector<Slot> slots_;
        std::vector<uint32_t> freeList_;
    };

    template <typename T, typename Tag>
    Handle<Tag> ResourcePool<T, Tag>::Insert(T value)
    {
        if (freeList_.empty())
        {
            slots_.emplace_back(std::move(value), 0, true);
            return Handle<Tag>{ static_cast<uint32_t>(slots_.size() - 1), 0 };
        }

        std::uint32_t index = freeList_.back();
        freeList_.pop_back();

        uint32_t generation = slots_[index].generation;

        slots_[index].value = std::move(value);
        slots_[index].alive = true;

        return Handle<Tag>(index, generation);
    }

    template <typename T, typename Tag>
    T* ResourcePool<T, Tag>::Get(Handle<Tag> h)
    {
        GEF_CORE_ASSERT(IsValid(h));

        if (!IsValid(h))
            return nullptr;

        return &slots_[h.index].value;
    }

    template <typename T, typename Tag>
    const T* ResourcePool<T, Tag>::Get(Handle<Tag> h) const
    {
        GEF_CORE_ASSERT(IsValid(h));

        if (!IsValid(h))
            return nullptr;

        return &slots_[h.index].value;
    }

    template <typename T, typename Tag>
    bool ResourcePool<T, Tag>::IsValid(Handle<Tag> h) const
    {
        return h.index < slots_.size() && slots_[h.index].alive && slots_[h.
            index].generation == h.generation;
    }

    template <typename T, typename Tag>
    void ResourcePool<T, Tag>::Remove(Handle<Tag> h)
    {
        GEF_CORE_ASSERT(IsValid(h));
        freeList_.push_back(h.index);

        slots_[h.index].alive = false;
        slots_[h.index].value = {};
        slots_[h.index].generation = slots_[h.index].generation + 1;
    }

    template <typename T, typename Tag>
    std::vector<Handle<Tag> > ResourcePool<T, Tag>::GetAliveHandles() const
    {
        std::vector<Handle<Tag> > ret;
        for (uint32_t i = 0; i < slots_.size(); ++i)
        {
            if (slots_[i].alive)
            {
                ret.emplace_back(Handle<Tag>(i, slots_[i].generation));
            }
        }
        return ret;
    }
}