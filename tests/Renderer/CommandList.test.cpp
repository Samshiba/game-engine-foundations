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
#include <variant>
#include <vector>

#include <doctest.h>

#include <Engine/Renderer/LightingData.hpp>

using namespace GEF::Renderer;

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
    cmd.BindUniformBuffer(UniformBinding::Light, Buffer(4));
    cmd.DrawIndexedInstanced(36, 10, 5);

    REQUIRE(cmd.GetDrawPackets().size() == 1);
    const auto& packet = cmd.GetDrawPackets()[0];
    CHECK(packet.pipeline == Pipeline(1));
    CHECK(packet.vertexArray == VertexArray(2));
    CHECK(packet.uniformBuffers[UniformBinding::Frame] == Buffer(3));
    CHECK(packet.uniformBuffers[UniformBinding::Light] == Buffer(4));
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
    cmd.DrawIndexed(3);

    REQUIRE(cmd.GetDrawPackets().size() == 2);
    CHECK(cmd.GetDrawPackets()[0].vertexArray == VertexArray(2));
    CHECK_FALSE(cmd.GetDrawPackets()[0]
                    .uniformBuffers[UniformBinding::Frame]
                    .IsValid());
    // ...while the next draw sees the new state
    CHECK(cmd.GetDrawPackets()[1].vertexArray == VertexArray(7));
    CHECK(cmd.GetDrawPackets()[1].uniformBuffers[UniformBinding::Frame]
          == Buffer(9));
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

TEST_CASE("Frame commands keep their values and their order")
{
    CommandList cmd;
    cmd.SetViewport(0, 0, 1280, 720);
    cmd.SetClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    cmd.Clear();

    const auto& commands = cmd.GetFrameCommands();
    REQUIRE(commands.size() == 3);

    const auto* viewport =
        std::get_if<CommandList::SetViewportCommand>(&commands[0]);
    REQUIRE(viewport != nullptr);
    CHECK(viewport->width == 1280);
    CHECK(viewport->height == 720);

    const auto* color =
        std::get_if<CommandList::SetClearColorCommand>(&commands[1]);
    REQUIRE(color != nullptr);
    CHECK(color->r == 0.1f);
    CHECK(color->b == 0.3f);

    CHECK(std::holds_alternative<CommandList::ClearCommand>(commands[2]));
}

TEST_CASE("Reset empties the list but keeps its memory")
{
    CommandList cmd;
    for (int i = 0; i < 100; ++i)
        RecordDraw(cmd, 1, 1);
    cmd.Clear();
    const auto packetCapacity = cmd.GetDrawPackets().capacity();
    const auto keyCapacity = cmd.GetSortKeys().capacity();

    cmd.Reset();

    CHECK(cmd.GetDrawPackets().empty());
    CHECK(cmd.GetSortKeys().empty());
    CHECK(cmd.GetFrameCommands().empty());
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
