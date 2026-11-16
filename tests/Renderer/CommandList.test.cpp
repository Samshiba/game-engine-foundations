//
// Path: tests/Renderer/CommandList.test.cpp
//
// Pure recording, no GPU: handles are plain values here, nothing is executed.
// A draw copies the state bound at that moment into a self-contained packet,
// and the sort keys order the packets by pass, then pipeline, then vertex
// array, keeping the recording order for equal keys.
//

#include <Engine/Renderer/CommandList.hpp>

#include <cstdint>
#include <vector>

#include <doctest.h>

#include <Engine/Renderer/Layout/LitShaderLayout.hpp>

using namespace GEF::Renderer;
using namespace GEF::Renderer::Layout;

namespace
{
    PipelineHandle Pipeline(uint32_t index)
    {
        return { .index = index, .generation = 0 };
    }

    VertexArrayHandle VertexArray(uint32_t index)
    {
        return { .index = index, .generation = 0 };
    }

    BufferHandle Buffer(uint32_t index)
    {
        return { .index = index, .generation = 0 };
    }

    void RecordDraw(CommandList& cmd, uint32_t pipeline, uint32_t vertexArray,
                    uint32_t indexCount = 3)
    {
        cmd.BindPipeline(Pipeline(pipeline));
        cmd.BindVertexArray(VertexArray(vertexArray));
        cmd.DrawIndexed(indexCount);
    }

    // The packets in the order Submit replays them
    std::vector<CommandList::DrawPacket> SortedPackets(CommandList& cmd)
    {
        cmd.SortPackets();
        std::vector<CommandList::DrawPacket> sorted;
        for (const uint64_t key : cmd.GetSortKeys())
            sorted.push_back(cmd.GetDrawPackets()[SortKey::Sequence(key)]);
        return sorted;
    }
} // namespace

TEST_CASE("Binding without drawing records no packet")
{
    CommandList cmd;
    cmd.BindPipeline(Pipeline(1));
    cmd.BindVertexArray(VertexArray(2));
    cmd.BindUniformBuffer(UniformBinding::Frame, Buffer(3));

    CHECK(cmd.GetDrawPackets().empty());
    CHECK(cmd.GetSortKeys().empty());
}

TEST_CASE("A draw copies the whole bound state into its packet")
{
    CommandList cmd;
    cmd.BindPipeline(Pipeline(1));
    cmd.BindVertexArray(VertexArray(2));
    cmd.BindUniformBuffer(UniformBinding::Frame, Buffer(3));
    cmd.BindUniformBuffer(UniformBinding::Environment, Buffer(4));
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::DirectionalLight,
                                Buffer(5));
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::PointLight, Buffer(6));
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::SpotLight, Buffer(7));
    cmd.DrawIndexedInstanced(36, 10, 5);

    REQUIRE(cmd.GetDrawPackets().size() == 1);
    const auto& packet = cmd.GetDrawPackets()[0];
    CHECK(packet.pipeline == Pipeline(1));
    CHECK(packet.vertexArray == VertexArray(2));
    CHECK(packet.uniformBuffers[UniformBinding::Frame] == Buffer(3));
    CHECK(packet.uniformBuffers[UniformBinding::Environment] == Buffer(4));
    CHECK(packet.shaderStorageBuffers[ShaderStorageBinding::DirectionalLight]
          == Buffer(5));
    CHECK(packet.shaderStorageBuffers[ShaderStorageBinding::PointLight]
          == Buffer(6));
    CHECK(packet.shaderStorageBuffers[ShaderStorageBinding::SpotLight]
          == Buffer(7));
    CHECK(packet.indexCount == 36);
    CHECK(packet.instanceCount == 10);
    CHECK(packet.baseInstance == 5);
}

TEST_CASE("A bind after a draw does not change that draw's packet")
{
    // Packets are snapshots: this is what makes reordering them safe
    CommandList cmd;
    RecordDraw(cmd, 1, 2);
    cmd.BindVertexArray(VertexArray(7));
    cmd.BindUniformBuffer(UniformBinding::Frame, Buffer(9));
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::PointLight, Buffer(10));
    cmd.DrawIndexed(3);

    REQUIRE(cmd.GetDrawPackets().size() == 2);
    CHECK(cmd.GetDrawPackets()[0].vertexArray == VertexArray(2));
    CHECK_FALSE(cmd.GetDrawPackets()[0]
                    .uniformBuffers[UniformBinding::Frame]
                    .IsValid());
    CHECK_FALSE(cmd.GetDrawPackets()[0]
                    .shaderStorageBuffers[ShaderStorageBinding::PointLight]
                    .IsValid());
    // ...while the next draw sees the new state
    CHECK(cmd.GetDrawPackets()[1].vertexArray == VertexArray(7));
    CHECK(cmd.GetDrawPackets()[1].uniformBuffers[UniformBinding::Frame]
          == Buffer(9));
    CHECK(cmd.GetDrawPackets()[1]
              .shaderStorageBuffers[ShaderStorageBinding::PointLight]
          == Buffer(10));
}

TEST_CASE("DrawIndexed is an instanced draw of one instance from slot 0")
{
    CommandList cmd;
    RecordDraw(cmd, 1, 1, 12);

    REQUIRE(cmd.GetDrawPackets().size() == 1);
    CHECK(cmd.GetDrawPackets()[0].indexCount == 12);
    CHECK(cmd.GetDrawPackets()[0].instanceCount == 1);
    CHECK(cmd.GetDrawPackets()[0].baseInstance == 0);
}

TEST_CASE("Sorting groups by pipeline first, then by vertex array")
{
    CommandList cmd;
    RecordDraw(cmd, 2, 1); // recorded first, but its pipeline sorts last
    RecordDraw(cmd, 1, 5);
    RecordDraw(cmd, 1, 2);

    const auto sorted = SortedPackets(cmd);

    REQUIRE(sorted.size() == 3);
    CHECK(sorted[0].pipeline == Pipeline(1));
    CHECK(sorted[0].vertexArray == VertexArray(2));
    CHECK(sorted[1].pipeline == Pipeline(1));
    CHECK(sorted[1].vertexArray == VertexArray(5));
    CHECK(sorted[2].pipeline == Pipeline(2));
}

TEST_CASE("Draws with the same key keep their recording order")
{
    // The sequence in the low bits breaks ties: std::sort becomes stable
    // without std::stable_sort's buffer allocation
    CommandList cmd;
    for (uint32_t indexCount = 3; indexCount <= 15; indexCount += 3)
        RecordDraw(cmd, 1, 1, indexCount);

    const auto sorted = SortedPackets(cmd);

    REQUIRE(sorted.size() == 5);
    for (uint32_t i = 0; i < 5; ++i)
    {
        CAPTURE(i);
        CHECK(sorted[i].indexCount == 3 * (i + 1));
    }
}

TEST_CASE("Sorting reorders the keys only, never the packets")
{
    CommandList cmd;
    RecordDraw(cmd, 2, 1);
    RecordDraw(cmd, 1, 1);

    cmd.SortPackets();

    // Packets stay in recording order: the keys index into them
    CHECK(cmd.GetDrawPackets()[0].pipeline == Pipeline(2));
    CHECK(SortKey::Sequence(cmd.GetSortKeys()[0]) == 1);
    CHECK(SortKey::Sequence(cmd.GetSortKeys()[1]) == 0);
}

TEST_CASE("Each key field outweighs every field after it")
{
    constexpr uint32_t MaxField12 = uint32_t(SortKey::FIELD_MASK_12);
    constexpr uint32_t MaxSequence = uint32_t(SortKey::SEQUENCE_MASK);

    // A higher pass beats the largest everything else
    CHECK(SortKey::Make(0, MaxField12, MaxField12, 0xFFFF, MaxSequence)
          < SortKey::Make(1, 0, 0, 0, 0));
    // A higher pipeline beats the largest vertex array, depth and sequence
    CHECK(SortKey::Make(0, 1, MaxField12, 0xFFFF, MaxSequence)
          < SortKey::Make(0, 2, 0, 0, 0));
    // A higher vertex array beats the largest depth and sequence
    CHECK(SortKey::Make(0, 1, 1, 0xFFFF, MaxSequence)
          < SortKey::Make(0, 1, 2, 0, 0));
    // Depth beats the sequence
    CHECK(SortKey::Make(0, 1, 1, 1, MaxSequence)
          < SortKey::Make(0, 1, 1, 2, 0));
}

TEST_CASE("The sequence comes back out of the key unchanged")
{
    for (const uint32_t sequence : { 0u, 1u, 4095u, 0xFFFFFu })
    {
        CAPTURE(sequence);
        CHECK(SortKey::Sequence(SortKey::Make(15, 4095, 4095, 0xFFFF, sequence))
              == sequence);
    }
}

TEST_CASE("The pass comes back out of the key unchanged")
{
    // Pass sits in the top 4 bits: reading the low bits would return a piece
    // of the sequence instead
    for (const uint32_t pass : { 0u, 1u, 7u, 15u })
    {
        CAPTURE(pass);
        CHECK(SortKey::Pass(SortKey::Make(pass, 4095, 4095, 0xFFFF, 0xFFFFF))
              == pass);
    }
}

TEST_CASE("A new list starts in the implicit back buffer pass")
{
    // A game that never calls BeginPass still draws to the screen
    CommandList cmd;

    const auto& passes = cmd.GetPasses();
    REQUIRE(passes.size() == 1);
    CHECK_FALSE(passes[0].desc.target.IsValid());
    CHECK(passes[0].desc.clearColor);
    CHECK(passes[0].desc.clearDepth);
    CHECK(passes[0].desc.clearValue.r == doctest::Approx(0.1f));
    CHECK_FALSE(passes[0].blitSource.IsValid());
}

TEST_CASE("BeginPass records the pass description as given")
{
    CommandList cmd;
    const RenderTargetHandle target{ .index = 3, .generation = 1 };

    cmd.BeginPass({ .target = target,
                    .clearColor = false,
                    .clearDepth = true,
                    .clearValue = { 0.25f, 0.5f, 0.75f, 1.0f } });
    cmd.EndPass();

    const auto& passes = cmd.GetPasses();
    REQUIRE(passes.size() == 2);
    CHECK(passes[1].desc.target == target);
    CHECK_FALSE(passes[1].desc.clearColor);
    CHECK(passes[1].desc.clearDepth);
    CHECK(passes[1].desc.clearValue.b == doctest::Approx(0.75f));
}

TEST_CASE("Each draw carries the index of the pass it was recorded in")
{
    CommandList cmd;
    RecordDraw(cmd, 1, 1); // implicit pass 0
    cmd.BeginPass({ .target = { .index = 0, .generation = 0 } });
    RecordDraw(cmd, 1, 1);
    RecordDraw(cmd, 1, 1);
    cmd.EndPass();
    cmd.BeginPass({});
    RecordDraw(cmd, 1, 1);
    cmd.EndPass();

    const auto& keys = cmd.GetSortKeys();
    REQUIRE(keys.size() == 4);
    CHECK(SortKey::Pass(keys[0]) == 0);
    CHECK(SortKey::Pass(keys[1]) == 1);
    CHECK(SortKey::Pass(keys[2]) == 1);
    CHECK(SortKey::Pass(keys[3]) == 2);
}

TEST_CASE("Sorting never moves a draw into another pass")
{
    // The pass outweighs the pipeline: a cheap pipeline index recorded in a
    // later pass must not be replayed before an earlier pass
    CommandList cmd;
    RecordDraw(cmd, 9, 9);
    cmd.BeginPass({});
    RecordDraw(cmd, 0, 0);
    cmd.EndPass();

    const auto sorted = SortedPackets(cmd);
    REQUIRE(sorted.size() == 2);
    CHECK(sorted[0].pipeline == Pipeline(9));
    CHECK(sorted[1].pipeline == Pipeline(0));
}

TEST_CASE("Blit sets the source of the current pass")
{
    CommandList cmd;
    const RenderTargetHandle scene{ .index = 2, .generation = 0 };

    cmd.BeginPass({ .clearColor = false, .clearDepth = false });
    cmd.Blit(scene);
    cmd.EndPass();

    const auto& passes = cmd.GetPasses();
    REQUIRE(passes.size() == 2);
    CHECK_FALSE(passes[0].blitSource.IsValid());
    CHECK(passes[1].blitSource == scene);
}

TEST_CASE("Reset empties the list but keeps its memory")
{
    CommandList cmd;
    for (int i = 0; i < 100; ++i)
        RecordDraw(cmd, 1, 1);
    cmd.BeginPass({});
    cmd.EndPass();
    const auto packetCapacity = cmd.GetDrawPackets().capacity();
    const auto keyCapacity = cmd.GetSortKeys().capacity();

    cmd.Reset();

    CHECK(cmd.GetDrawPackets().empty());
    CHECK(cmd.GetSortKeys().empty());
    // Only the implicit pass is left
    REQUIRE(cmd.GetPasses().size() == 1);
    CHECK_FALSE(cmd.GetPasses()[0].desc.target.IsValid());
    // No allocation next frame: the steady state is allocation-free
    CHECK(cmd.GetDrawPackets().capacity() == packetCapacity);
    CHECK(cmd.GetSortKeys().capacity() == keyCapacity);
}

TEST_CASE("Reset forgets the bound uniform buffers")
{
    // A buffer from the previous frame (maybe destroyed since) must not leak
    // into this frame's packets
    CommandList cmd;
    cmd.BindUniformBuffer(UniformBinding::Frame, Buffer(3));
    cmd.Reset();

    RecordDraw(cmd, 1, 1);

    REQUIRE(cmd.GetDrawPackets().size() == 1);
    CHECK_FALSE(cmd.GetDrawPackets()[0]
                    .uniformBuffers[UniformBinding::Frame]
                    .IsValid());
}

TEST_CASE("Reset forgets the bound shader storage buffers")
{
    // Same reason as the uniform buffers: a light SSBO regrown (destroyed and
    // recreated) last frame must not reach this frame's packets
    CommandList cmd;
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::PointLight, Buffer(3));
    cmd.Reset();

    RecordDraw(cmd, 1, 1);

    REQUIRE(cmd.GetDrawPackets().size() == 1);
    CHECK_FALSE(cmd.GetDrawPackets()[0]
                    .shaderStorageBuffers[ShaderStorageBinding::PointLight]
                    .IsValid());
}

TEST_CASE("Uniform and shader storage bindings are separate slots")
{
    // GL_UNIFORM_BUFFER 0 and GL_SHADER_STORAGE_BUFFER 0 are two different
    // binding points: binding the same index in both overwrites neither
    CommandList cmd;
    cmd.BindUniformBuffer(0, Buffer(1));
    cmd.BindShaderStorageBuffer(0, Buffer(2));
    RecordDraw(cmd, 1, 1);

    REQUIRE(cmd.GetDrawPackets().size() == 1);
    CHECK(cmd.GetDrawPackets()[0].uniformBuffers[0] == Buffer(1));
    CHECK(cmd.GetDrawPackets()[0].shaderStorageBuffers[0] == Buffer(2));
}
