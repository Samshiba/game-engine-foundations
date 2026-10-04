//
// Created by genin on 04/10/2026.
// Path: tests/Renderer/ResourcePool.test.cpp
//

#include <doctest.h>

#include <Engine/Renderer/ResourcePool.hpp>

#include <cstdint>

using namespace GEF::Renderer;

namespace
{
    // A struct, not an int: an int would silently accept assignments a real
    // backend resource type would reject
    struct FakeBuffer
    {
        std::uint32_t id;
        std::uint32_t size;
    };

    using Pool = ResourcePool<FakeBuffer, BufferTag>;
}

TEST_CASE("A default handle is invalid")
{
    Pool pool;
    const BufferHandle handle;

    CHECK_FALSE(pool.IsValid(handle));
}

TEST_CASE("Insert returns a valid handle to the stored value")
{
    Pool pool;
    const BufferHandle handle = pool.Insert({ 7, 64 });

    REQUIRE(pool.IsValid(handle));
    CHECK(pool.Get(handle)->id == 7);
    CHECK(pool.Get(handle)->size == 64);
}

TEST_CASE("Handles stay distinct and valid")
{
    Pool pool;
    const BufferHandle a = pool.Insert({ 1, 0 });
    const BufferHandle b = pool.Insert({ 2, 0 });

    CHECK(a.index != b.index);
    CHECK(pool.Get(a)->id == 1);
    CHECK(pool.Get(b)->id == 2);
}

TEST_CASE("Remove invalidates the handle")
{
    Pool pool;
    const BufferHandle handle = pool.Insert({ 1, 0 });
    pool.Remove(handle);

    CHECK_FALSE(pool.IsValid(handle));
}

TEST_CASE("A freed slot is reused with a new generation")
{
    Pool pool;
    const BufferHandle a = pool.Insert({ 1, 0 });
    const BufferHandle b = pool.Insert({ 2, 0 });
    const BufferHandle c = pool.Insert({ 3, 0 });

    pool.Remove(b);
    const BufferHandle d = pool.Insert({ 4, 0 });

    CHECK(d.index == b.index);
    CHECK(d.generation == b.generation + 1);

    SUBCASE("the stale handle is rejected, the new one works")
    {
        CHECK_FALSE(pool.IsValid(b));
        REQUIRE(pool.IsValid(d));
        CHECK(pool.Get(d)->id == 4);
    }

    SUBCASE("the other handles are untouched")
    {
        CHECK(pool.Get(a)->id == 1);
        CHECK(pool.Get(c)->id == 3);
    }
}

TEST_CASE("Several freed slots are all reused before the pool grows")
{
    Pool pool;
    const BufferHandle a = pool.Insert({ 1, 0 });
    const BufferHandle b = pool.Insert({ 2, 0 });
    pool.Insert({ 3, 0 });

    pool.Remove(a);
    pool.Remove(b);

    const BufferHandle x = pool.Insert({ 4, 0 });
    const BufferHandle y = pool.Insert({ 5, 0 });
    const BufferHandle z = pool.Insert({ 6, 0 }); // no free slot left: grows

    CHECK(((x.index == a.index && y.index == b.index) ||
        (x.index == b.index && y.index == a.index)));
    CHECK(z.index == 3);
}

TEST_CASE("GetAliveHandles lists exactly the live resources")
{
    Pool pool;
    const BufferHandle a = pool.Insert({ 1, 0 });
    const BufferHandle b = pool.Insert({ 2, 0 });
    const BufferHandle c = pool.Insert({ 3, 0 });
    pool.Remove(b);

    const auto alive = pool.GetAliveHandles();

    REQUIRE(alive.size() == 2);
    for (const BufferHandle& handle : alive)
    {
        CHECK(pool.IsValid(handle));
        CHECK((handle.index == a.index || handle.index == c.index));
    }
}