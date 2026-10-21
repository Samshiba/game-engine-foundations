//
// Path: tests/Renderer/VertexLayouts.test.cpp
//
// A vertex layout describes a C++ struct to the GPU. If a field is added to
// the struct but not to its Layout() (or the other way around), every vertex
// or instance after the first is read at the wrong place: garbage on screen,
// no error. No GPU needed: the stride must equal the struct size.
//

#include <doctest.h>

#include <Engine/Renderer/Mesh.hpp>

using namespace GEF::Renderer;

TEST_CASE("Vertex::Layout() describes the whole Vertex struct")
{
    CHECK(Vertex::Layout().GetStride() == sizeof(Vertex));
}

TEST_CASE("InstanceData::Layout() describes the whole InstanceData struct")
{
    CHECK(InstanceData::Layout().GetStride() == sizeof(InstanceData));
}

TEST_CASE("Mesh and instance attributes fit in the 16 attribute slots")
{
    // A matrix takes one attribute slot per column
    const auto slots = [](const BufferLayout& layout) {
        int count = 0;
        for (const BufferElement& element : layout)
        {
            const bool isMatrix = element.type == ShaderDataType::Mat3 ||
                element.type == ShaderDataType::Mat4;
            count += isMatrix ? element.GetComponentCount() : 1;
        }
        return count;
    };

    const int total = slots(Vertex::Layout()) + slots(InstanceData::Layout());
    CHECK(total == 12);   // 3 per vertex + 4 (model) + 3 (normal) + 2
    CHECK(total <= 16);   // GL_MAX_VERTEX_ATTRIBS is at least 16
}
