//
// Created by genin on 15/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.cpp
//

#include "OpenGLGraphicsDevice.hpp"
#include "OpenGLCommandList.hpp"

namespace GEF::Platform
{
    OpenGLGraphicsDevice::~OpenGLGraphicsDevice()
    {
        // Clean vertex arrays first, they reference buffers
        const auto vertexArrays = vertexArrays_.GetAliveHandles();
        if (!vertexArrays.empty())
        {
            GEF_ENGINE_WARN("{} vertex arrays leaked! Destroy them!",
                            vertexArrays.size());
            for (const auto& handle : vertexArrays)
                OpenGLGraphicsDevice::DestroyVertexArray(handle);
        }

        const auto buffers = buffers_.GetAliveHandles();
        if (!buffers.empty())
        {
            GEF_ENGINE_WARN("{} buffers leaked! Destroy them!", buffers.size());
            for (const auto& handle : buffers)
                OpenGLGraphicsDevice::DestroyBuffer(handle);
        }

        const auto shaders = shaders_.GetAliveHandles();
        if (!shaders.empty())
        {
            GEF_ENGINE_WARN("{} shaders leaked! Destroy them!", shaders.size());
            for (const auto& handle : shaders)
                OpenGLGraphicsDevice::DestroyShader(handle);
        }

        const auto pipelines = pipelines_.GetAliveHandles();
        if (!pipelines.empty())
        {
            GEF_ENGINE_WARN("{} pipelines leaked! Destroy them!",
                            pipelines.size());
            for (const auto& handle : pipelines)
                OpenGLGraphicsDevice::DestroyPipeline(handle);
        }
    }

    Renderer::BufferHandle OpenGLGraphicsDevice::CreateBuffer(
        const Renderer::BufferDesc& desc)
    {
        GLuint id = 0;
        glCreateBuffers(1, &id);

        switch (desc.usage)
        {
        case Renderer::BufferUsage::Static:
            glNamedBufferData(id, desc.size, desc.data, GL_STATIC_DRAW);
            break;
        case Renderer::BufferUsage::Dynamic:
            glNamedBufferData(id, desc.size, desc.data, GL_DYNAMIC_DRAW);
            break;
        case Renderer::BufferUsage::Stream:
            glNamedBufferData(id, desc.size, desc.data, GL_STREAM_DRAW);
            break;
        default:
            GEF_CORE_ASSERT(false, "Invalid buffer usage");
        }

        return buffers_.Insert(GLBuffer{ id, desc.size, desc.type });
    }

    void OpenGLGraphicsDevice::DestroyBuffer(Renderer::BufferHandle handle)
    {
        auto buffer = buffers_.Get(handle);
        if (buffer)
        {
            glDeleteBuffers(1, &buffer->id);
            buffers_.Remove(handle);
        }
    }

    bool OpenGLGraphicsDevice::UpdateBuffer(Renderer::BufferHandle handle,
                                            uint32_t offset, const void* data,
                                            uint32_t size)
    {
        auto buffer = buffers_.Get(handle);
        if (!buffer)
            return false;

        GEF_CORE_ASSERT(offset + size <= buffer->size,
                        "UpdateBuffer out of range");

        glNamedBufferSubData(buffer->id, offset, size, data);
        return true;
    }

    Renderer::VertexArrayHandle OpenGLGraphicsDevice::CreateVertexArray(
        const Renderer::VertexArrayDesc& desc)
    {
        GLuint vao = 0;
        glCreateVertexArrays(1, &vao);

        // Attribute = one shader input (layout(location = N)): format + offset.
        // Binding   = one buffer slot: which buffer, stride, per-vertex or
        //             per-instance. Each attribute reads from one binding.
        GLuint attrib = 0;
        for (GLuint b = 0; b < desc.vertexBuffers.size(); ++b)
        {
            const auto& binding = desc.vertexBuffers[b];
            const GLBuffer* buffer = GetBuffer(binding.buffer);
            GEF_CORE_ASSERT(buffer, "Invalid vertex buffer handle");
            GEF_CORE_ASSERT(buffer->type == Renderer::BufferType::Vertex,
                            "Buffer is not a vertex buffer");

            glVertexArrayVertexBuffer(vao, b, buffer->id, 0,
                                      static_cast<GLsizei>(
                                          binding.layout.GetStride()));
            glVertexArrayBindingDivisor(vao, b, binding.instanceDivisor);

            const auto addAttribute = [&](GLint count, GLenum type,
                                          bool isInteger, bool normalized,
                                          size_t offset) {
                const auto relativeOffset = static_cast<GLuint>(offset);
                glEnableVertexArrayAttrib(vao, attrib);
                if (isInteger)
                    glVertexArrayAttribIFormat(vao, attrib, count, type,
                                               relativeOffset);
                else
                    glVertexArrayAttribFormat(vao, attrib, count, type,
                                              normalized ? GL_TRUE : GL_FALSE,
                                              relativeOffset);
                glVertexArrayAttribBinding(vao, attrib, b);
                ++attrib;
            };

            for (const Renderer::BufferElement& element : binding.layout)
            {
                const GLint count = element.GetComponentCount();
                switch (element.type)
                {
                    using enum Renderer::ShaderDataType;
                case Float:
                case Float2:
                case Float3:
                case Float4:
                    addAttribute(count, GL_FLOAT, false, element.normalized,
                                 element.offset);
                    break;
                case Int:
                case Int2:
                case Int3:
                case Int4:
                case Bool:
                    addAttribute(count, GL_INT, true, false, element.offset);
                    break;
                case Mat3:
                case Mat4:
                    for (GLint column = 0; column < count; ++column)
                        addAttribute(count, GL_FLOAT, false, element.normalized,
                                     element.offset + sizeof(float) * count *
                                     column);
                    break;
                default:
                    GEF_CORE_ASSERT(false, "Unsupported data type");
                }
            }
        }

        const GLBuffer* indexBuffer = GetBuffer(desc.indexBuffer);
        GEF_CORE_ASSERT(indexBuffer, "Invalid index buffer handle");
        GEF_CORE_ASSERT(indexBuffer->type == Renderer::BufferType::Index,
                        "Buffer is not an index buffer");
        glVertexArrayElementBuffer(vao, indexBuffer->id);

        return vertexArrays_.Insert(GLVertexArray{ vao });
    }

    void OpenGLGraphicsDevice::DestroyVertexArray(
        Renderer::VertexArrayHandle handle)
    {
        auto vertexArray = vertexArrays_.Get(handle);
        if (vertexArray)
        {
            glDeleteVertexArrays(1, &vertexArray->id);
            vertexArrays_.Remove(handle);
        }
    }

    namespace
    {
        GLuint CompileStage(GLenum stage, const std::string_view source)
        {
            const char* data = source.data();
            const auto length = static_cast<GLint>(source.size());
            const GLuint shader = glCreateShader(stage);
            glShaderSource(shader, 1, &data, &length);
            glCompileShader(shader);

            GLint success = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                GLint maxLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

                std::string infoLog(maxLength, '\0');
                glGetShaderInfoLog(shader, maxLength, nullptr, infoLog.data());
                GEF_ENGINE_ERROR("Shader compilation failed: {}", infoLog);

                glDeleteShader(shader);
                return 0;
            }
            return shader;
        }
    }

    Renderer::ShaderHandle OpenGLGraphicsDevice::CreateShader(
        const Renderer::ShaderDesc& desc)
    {
        const GLuint vs = CompileStage(
            GL_VERTEX_SHADER, desc.vertexSource);
        if (!vs)
            return Renderer::ShaderHandle{};
        const GLuint fs = CompileStage(
            GL_FRAGMENT_SHADER, desc.fragmentSource);
        if (!fs)
        {
            glDeleteShader(vs);
            return Renderer::ShaderHandle{};
        }

        const GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        GLint success = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            std::string log(maxLength, '\0');
            glGetProgramInfoLog(program, maxLength, nullptr, log.data());
            GEF_ENGINE_ERROR("Shader link failed: {}", log);

            glDeleteShader(vs);
            glDeleteShader(fs);
            glDeleteProgram(program);

            return Renderer::ShaderHandle{};
        }
        glDeleteShader(vs);
        glDeleteShader(fs);

        return shaders_.Insert(GLShader{ program });
    }

    void OpenGLGraphicsDevice::DestroyShader(Renderer::ShaderHandle handle)
    {
        auto shader = shaders_.Get(handle);
        if (shader)
        {
            glDeleteProgram(shader->program);
            shaders_.Remove(handle);
        }
    }

    Renderer::PipelineHandle OpenGLGraphicsDevice::CreatePipeline(
        const Renderer::PipelineDesc& desc)
    {
        GEF_CORE_ASSERT(desc.shader.IsValid(), "Pipeline needs a valid shader");

        if (!GetShader(desc.shader))
            return Renderer::PipelineHandle{};

        return pipelines_.Insert(
            GLPipeline{ desc.shader, desc.depth, desc.cull });
    }

    void OpenGLGraphicsDevice::DestroyPipeline(Renderer::PipelineHandle handle)
    {
        pipelines_.Remove(handle);
    }

    const GLBuffer* OpenGLGraphicsDevice::GetBuffer(
        Renderer::BufferHandle handle) const
    {
        return buffers_.IsValid(handle) ? buffers_.Get(handle) : nullptr;
    }

    const GLVertexArray* OpenGLGraphicsDevice::GetVertexArray(
        Renderer::VertexArrayHandle handle) const
    {
        return vertexArrays_.IsValid(handle)
            ? vertexArrays_.Get(handle)
            : nullptr;
    }

    const GLShader* OpenGLGraphicsDevice::GetShader(
        Renderer::ShaderHandle handle) const
    {
        return shaders_.IsValid(handle) ? shaders_.Get(handle) : nullptr;
    }

    const GLPipeline* OpenGLGraphicsDevice::GetPipeline(
        Renderer::PipelineHandle handle) const
    {
        return pipelines_.IsValid(handle) ? pipelines_.Get(handle) : nullptr;
    }

    std::shared_ptr<Renderer::CommandList> OpenGLGraphicsDevice::
    BeginCommandList()
    {
        return std::make_shared<OpenGLCommandList>(*this);
    }

    void OpenGLGraphicsDevice::SubmitCommandList(
        const std::shared_ptr<Renderer::CommandList>& commandList)
    {
        (void)commandList;
    }
}