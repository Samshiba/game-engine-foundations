//
// Created by genin on 15/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.cpp
//

#include "OpenGLGraphicsDevice.hpp"

#include <algorithm>
#include <bit>
#include <chrono>

#include <Engine/Utils/Overloaded.hpp>

namespace GEF::Platform
{
    namespace
    {
        std::string GetGLString(GLenum name)
        {
            const auto* value =
                reinterpret_cast<const char*>(glGetString(name));
            return value ? value : "unknown";
        }
    } // namespace

    OpenGLGraphicsDevice::OpenGLGraphicsDevice()
        : adapterInfo_{ .vendor = GetGLString(GL_VENDOR),
                        .renderer = GetGLString(GL_RENDERER),
                        .version = GetGLString(GL_VERSION) }
    {
        GEF_ENGINE_INFO("GPU: {} ({}), OpenGL {}", adapterInfo_.renderer,
                        adapterInfo_.vendor, adapterInfo_.version);

        for (auto& gpuTimer : gpuTimers_)
        {
            glCreateQueries(GL_TIMESTAMP, 1, &gpuTimer.start);
            glCreateQueries(GL_TIMESTAMP, 1, &gpuTimer.end);
        }
    }

    const Renderer::AdapterInfo& OpenGLGraphicsDevice::GetAdapterInfo() const
    {
        return adapterInfo_;
    }

    const Renderer::SubmitStats&
    OpenGLGraphicsDevice::GetLastSubmitStats() const
    {
        return lastSubmitStats_;
    }

    OpenGLGraphicsDevice::~OpenGLGraphicsDevice()
    {
        for (const auto& gpuTimer : gpuTimers_)
        {
            glDeleteQueries(1, &gpuTimer.start);
            glDeleteQueries(1, &gpuTimer.end);
        }
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

        const auto renderTargets = renderTargets_.GetAliveHandles();
        if (!renderTargets.empty())
        {
            GEF_ENGINE_WARN("{} render targets leaked! Destroy them!",
                            renderTargets.size());
            for (const auto& handle : renderTargets)
                OpenGLGraphicsDevice::DestroyRenderTarget(handle);
        }

        const auto textures = textures_.GetAliveHandles();
        if (!textures.empty())
        {
            GEF_ENGINE_WARN("{} textures leaked! Destroy them!",
                            textures.size());
            for (const auto& handle : textures)
                OpenGLGraphicsDevice::DestroyTexture(handle);
        }
    }

    Renderer::BufferHandle
    OpenGLGraphicsDevice::CreateBuffer(const Renderer::BufferDesc& desc)
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

            glVertexArrayVertexBuffer(
                vao, b, buffer->id, 0,
                static_cast<GLsizei>(binding.layout.GetStride()));
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
                                     element.offset
                                         + sizeof(float) * count * column);
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

    void
    OpenGLGraphicsDevice::DestroyVertexArray(Renderer::VertexArrayHandle handle)
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
                GLsizei written = 0;
                glGetShaderInfoLog(shader, maxLength, &written, infoLog.data());
                infoLog.resize(written);
                GEF_ENGINE_ERROR("Shader compilation failed: {}", infoLog);

                glDeleteShader(shader);
                return 0;
            }
            return shader;
        }
    } // namespace

    Renderer::ShaderHandle
    OpenGLGraphicsDevice::CreateShader(const Renderer::ShaderDesc& desc)
    {
        const GLuint vs = CompileStage(GL_VERTEX_SHADER, desc.vertexSource);
        if (!vs)
            return Renderer::ShaderHandle{};
        const GLuint fs = CompileStage(GL_FRAGMENT_SHADER, desc.fragmentSource);
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
            GLsizei written = 0;
            glGetProgramInfoLog(program, maxLength, &written, log.data());
            log.resize(written);
            GEF_ENGINE_ERROR("Shader link failed: {}", log);

            glDeleteShader(vs);
            glDeleteShader(fs);
            glDeleteProgram(program);

            return Renderer::ShaderHandle{};
        }
        glDeleteShader(vs);
        glDeleteShader(fs);

        GEF_ENGINE_DEBUG("Shader created successfully: {}", program);

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

    Renderer::PipelineHandle
    OpenGLGraphicsDevice::CreatePipeline(const Renderer::PipelineDesc& desc)
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

    Renderer::TextureHandle
    OpenGLGraphicsDevice::CreateTexture(const Renderer::TextureDesc& desc)
    {
        if (desc.width == 0 || desc.height == 0)
            return Renderer::TextureHandle{};

        GLenum internalFormat = 0;
        GLenum uploadFormat = 0;
        GLenum uploadType = 0;
        GLint wrap = 0;

        switch (desc.format)
        {
        case Renderer::TextureFormat::RGBA8:
            internalFormat = GL_RGBA8;
            uploadFormat = GL_RGBA;
            uploadType = GL_UNSIGNED_BYTE;
            break;
        case Renderer::TextureFormat::SRGBA8:
            internalFormat = GL_SRGB8_ALPHA8;
            uploadFormat = GL_RGBA;
            uploadType = GL_UNSIGNED_BYTE;
            break;
        case Renderer::TextureFormat::Depth32F:
            internalFormat = GL_DEPTH_COMPONENT32F;
            uploadFormat = GL_DEPTH_COMPONENT;
            uploadType = GL_FLOAT;
            break;
        case Renderer::TextureFormat::None:
        default:
            return Renderer::TextureHandle{};
        }

        switch (desc.wrap)
        {
        case Renderer::TextureWrap::ClampToEdge:
            wrap = GL_CLAMP_TO_EDGE;
            break;
        case Renderer::TextureWrap::ClampToBorder:
            wrap = GL_CLAMP_TO_BORDER;
            break;
        case Renderer::TextureWrap::Repeat:
            wrap = GL_REPEAT;
            break;
        case Renderer::TextureWrap::MirroredRepeat:
            wrap = GL_MIRRORED_REPEAT;
            break;
        }

        bool useMipmaps = desc.generateMipmaps
            && !(desc.format == Renderer::TextureFormat::Depth32F);
        int levels =
            useMipmaps ? std::bit_width(std::max(desc.width, desc.height)) : 1;

        GLTexture texture{ .id = 0,
                           .width = desc.width,
                           .height = desc.height,
                           .internalFormat = internalFormat,
                           .levels = levels,
                           .wrapMode = wrap };
        if (!AllocateTextureStorage(texture))
        {
            return Renderer::TextureHandle{};
        }

        if (desc.data)
        {
            glTextureSubImage2D(texture.id, 0, 0, 0, desc.width, desc.height,
                                uploadFormat, uploadType, desc.data);
            if (useMipmaps)
            {
                glGenerateTextureMipmap(texture.id);
            }
        }

        return textures_.Insert(texture);
    }

    void OpenGLGraphicsDevice::DestroyTexture(Renderer::TextureHandle handle)
    {
        auto texture = textures_.Get(handle);
        if (texture)
        {
            glDeleteTextures(1, &texture->id);
            textures_.Remove(handle);
        }
    }

    Renderer::RenderTargetHandle OpenGLGraphicsDevice::CreateRenderTarget(
        const Renderer::RenderTargetDesc& desc)
    {
        // Init render target
        GLRenderTarget target{};
        target.desc = desc;
        const auto size = ComputeSize(desc);
        target.width = size.x;
        target.height = size.y;

        glCreateFramebuffers(1, &target.id);

        // Create color textures
        std::array<GLenum, Renderer::MaxColorAttachments> drawBuffers{};
        uint32_t colorAttachmentCount = 0;
        uint32_t lastColorAttachment = 0;
        for (uint32_t i = 0; i < Renderer::MaxColorAttachments; ++i)
        {
            if (desc.attachments[i] == Renderer::TextureFormat::None)
                continue;

            Renderer::TextureDesc texDesc{};
            texDesc.width = target.width;
            texDesc.height = target.height;
            texDesc.format = desc.attachments[i];
            texDesc.generateMipmaps = false;
            texDesc.wrap = Renderer::TextureWrap::ClampToEdge;

            Renderer::TextureHandle texHandle = CreateTexture(texDesc);
            if (!texHandle.IsValid())
            {
                GEF_ENGINE_ERROR("Failed to create render target texture");
                DestroyRenderTargetResources(target);
                return Renderer::RenderTargetHandle{};
            }

            target.attachments[i] = texHandle;

            drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
            glNamedFramebufferTexture(target.id, GL_COLOR_ATTACHMENT0 + i,
                                      GetTexture(texHandle)->id, 0);

            colorAttachmentCount++;
            lastColorAttachment = i;
        }

        if (colorAttachmentCount > 0)
        {
            glNamedFramebufferDrawBuffers(target.id, lastColorAttachment + 1,
                                          drawBuffers.data());
        }
        else
        {
            glNamedFramebufferDrawBuffer(target.id, GL_NONE);
            glNamedFramebufferReadBuffer(target.id, GL_NONE);
        }

        // Create depth texture
        if (desc.depthStencil != Renderer::TextureFormat::None)
        {
            Renderer::TextureDesc depthTexDesc{};
            depthTexDesc.width = target.width;
            depthTexDesc.height = target.height;
            depthTexDesc.format = desc.depthStencil;
            depthTexDesc.wrap = Renderer::TextureWrap::ClampToEdge;

            Renderer::TextureHandle depthTexHandle =
                CreateTexture(depthTexDesc);
            if (!depthTexHandle.IsValid())
            {
                GEF_ENGINE_ERROR(
                    "Failed to create render target depth texture");
                DestroyRenderTargetResources(target);
                return Renderer::RenderTargetHandle{};
            }

            target.depthStencil = depthTexHandle;
            glNamedFramebufferTexture(target.id, GL_DEPTH_ATTACHMENT,
                                      GetTexture(depthTexHandle)->id, 0);
        }

        // Creation check
        GLenum status =
            glCheckNamedFramebufferStatus(target.id, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            GEF_ENGINE_ERROR("Render target framebuffer is incomplete");
            DestroyRenderTargetResources(target);
            return Renderer::RenderTargetHandle{};
        }

        return renderTargets_.Insert(target);
    }

    void OpenGLGraphicsDevice::DestroyRenderTarget(
        Renderer::RenderTargetHandle handle)
    {
        auto renderTarget = renderTargets_.Get(handle);
        if (renderTarget)
        {
            DestroyRenderTargetResources(*renderTarget);
            renderTargets_.Remove(handle);
        }
    }

    Renderer::TextureHandle OpenGLGraphicsDevice::GetRenderTargetDepth(
        Renderer::RenderTargetHandle handle)
    {
        auto renderTarget = renderTargets_.Get(handle);
        if (renderTarget)
        {
            return renderTarget->depthStencil;
        }

        return Renderer::TextureHandle{};
    }

    const GLBuffer*
    OpenGLGraphicsDevice::GetBuffer(Renderer::BufferHandle handle) const
    {
        return buffers_.IsValid(handle) ? buffers_.Get(handle) : nullptr;
    }

    const GLVertexArray* OpenGLGraphicsDevice::GetVertexArray(
        Renderer::VertexArrayHandle handle) const
    {
        return vertexArrays_.IsValid(handle) ? vertexArrays_.Get(handle)
                                             : nullptr;
    }

    const GLShader*
    OpenGLGraphicsDevice::GetShader(Renderer::ShaderHandle handle) const
    {
        return shaders_.IsValid(handle) ? shaders_.Get(handle) : nullptr;
    }

    const GLPipeline*
    OpenGLGraphicsDevice::GetPipeline(Renderer::PipelineHandle handle) const
    {
        return pipelines_.IsValid(handle) ? pipelines_.Get(handle) : nullptr;
    }

    const GLTexture*
    OpenGLGraphicsDevice::GetTexture(Renderer::TextureHandle handle) const
    {
        return textures_.IsValid(handle) ? textures_.Get(handle) : nullptr;
    }

    const GLRenderTarget* OpenGLGraphicsDevice::GetRenderTarget(
        Renderer::RenderTargetHandle handle) const
    {
        return renderTargets_.IsValid(handle) ? renderTargets_.Get(handle)
                                              : nullptr;
    }

    void OpenGLGraphicsDevice::Submit(Renderer::CommandList& cmd)
    {
        cmd.SortPackets();
        lastSubmitStats_ = {};
        BoundState bound{};

        // ----- GPU and CPU fram time -----
        GpuTimer& timer = gpuTimers_[gpuTimerIndex_];
        gpuTimerIndex_ = (gpuTimerIndex_ + 1) % gpuTimers_.size();
        if (timer.pending)
        {
            GLint available = 0;
            glGetQueryObjectiv(timer.end, GL_QUERY_RESULT_AVAILABLE,
                               &available);
            if (available)
            {
                GLint64 start = 0;
                GLint64 end = 0;
                glGetQueryObjecti64v(timer.start, GL_QUERY_RESULT, &start);
                glGetQueryObjecti64v(timer.end, GL_QUERY_RESULT, &end);
                gpuFrameTimeMs_ = ((end - start) / 1'000'000.0);
            }
        }
        lastSubmitStats_.gpuFrameTimeMs = gpuFrameTimeMs_;
        glQueryCounter(timer.start, GL_TIMESTAMP);
        const auto cpuStart = std::chrono::steady_clock::now();

        // ----- Passes -----
        size_t k = 0;
        const auto& keys = cmd.GetSortKeys();
        for (uint32_t p = 0; p < cmd.GetPasses().size(); ++p)
        {
            const auto& pass = cmd.GetPasses()[p];
            auto target = GetRenderTarget(pass.desc.target);
            if (!target)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            else
            {
                glBindFramebuffer(GL_FRAMEBUFFER, target->id);
            }

            SetViewport(0, 0, target ? target->width : backBufferWidth_,
                        target ? target->height : backBufferHeight_);

            if (pass.desc.clearColor)
            {
                SetClearColor(pass.desc.clearValue.r, pass.desc.clearValue.g,
                              pass.desc.clearValue.b, pass.desc.clearValue.a);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            if (pass.desc.clearDepth)
            {
                glDepthMask(GL_TRUE);
                glClear(GL_DEPTH_BUFFER_BIT);
                bound.pipeline = {};
            }

            if (pass.blitSource.IsValid())
            {
                auto const source = GetRenderTarget(pass.blitSource);
                if (source)
                {
                    const GLuint dst = target ? target->id : 0;
                    const uint32_t dw =
                        target ? target->width : backBufferWidth_;
                    const uint32_t dh =
                        target ? target->height : backBufferHeight_;
                    glBlitNamedFramebuffer(
                        source->id, dst, 0, 0, source->width, source->height, 0,
                        0, dw, dh, GL_COLOR_BUFFER_BIT,
                        (source->width == dw && source->height == dh)
                            ? GL_NEAREST
                            : GL_LINEAR);
                }
            }

            // ----- Packets -----
            const auto& drawPackets = cmd.GetDrawPackets();
            for (; k < keys.size() && Renderer::SortKey::Pass(keys[k]) == p;
                 ++k)
            {
                const auto sequence = Renderer::SortKey::Sequence(keys[k]);
                auto const& packet = drawPackets[sequence];

                // Pipeline
                if (packet.pipeline != bound.pipeline)
                {
                    BindPipeline(packet.pipeline);
                    bound.pipeline = packet.pipeline;
                    ++lastSubmitStats_.pipelineChanges;
                }

                // Vertex
                if (packet.vertexArray != bound.vertexArray)
                {
                    BindVertexArray(packet.vertexArray);
                    bound.vertexArray = packet.vertexArray;
                    ++lastSubmitStats_.vertexArrayChanges;
                }

                // Uniform
                for (uint32_t i = 0;
                     i < Renderer::CommandList::MaxUniformBindings; ++i)
                {
                    const auto buffer = packet.uniformBuffers[i];
                    if (buffer.IsValid() && buffer != bound.uniformBuffers[i])
                    {
                        BindUniformBuffer(i, buffer);
                        bound.uniformBuffers[i] = buffer;
                        ++lastSubmitStats_.uniformBufferChanges;
                    }
                }

                // Shader Storage
                for (uint32_t i = 0;
                     i < Renderer::CommandList::MaxShaderStorageBindings; ++i)
                {
                    const auto buffer = packet.shaderStorageBuffers[i];
                    if (buffer.IsValid()
                        && buffer != bound.shaderStorageBuffers[i])
                    {
                        BindShaderStorageBuffer(i, buffer);
                        bound.shaderStorageBuffers[i] = buffer;
                        ++lastSubmitStats_.shaderStorageBufferChanges;
                    }
                }

                // Texture
                for (uint32_t i = 0;
                     i < Renderer::CommandList::MaxTextureBindings; ++i)
                {
                    const auto texture = packet.textures[i];
                    if (texture.IsValid() && texture != bound.textures[i])
                    {
                        BindTexture(i, texture);
                        bound.textures[i] = texture;
                        ++lastSubmitStats_.textureChanges;
                    }
                }

                DrawIndexedInstanced(packet.indexCount, packet.instanceCount,
                                     packet.baseInstance);
                ++lastSubmitStats_.drawCalls;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER,
                          0); // ImGui reset

        // Before the query readback: CPU can wait for the GPU, which is not
        // work
        lastSubmitStats_.cpuSubmitTimeMs =
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - cpuStart)
                .count();
        glQueryCounter(timer.end, GL_TIMESTAMP);
        timer.pending = true;
    }

    void OpenGLGraphicsDevice::SetClearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }

    void OpenGLGraphicsDevice::Clear()
    {
        glDepthMask(GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLGraphicsDevice::SetViewport(uint32_t x, uint32_t y,
                                           uint32_t width, uint32_t height)
    {
        glViewport(x, y, width, height);
    }

    void
    OpenGLGraphicsDevice::BindVertexArray(Renderer::VertexArrayHandle handle)
    {
        const GLVertexArray* vertexArray = GetVertexArray(handle);
        GEF_CORE_ASSERT(vertexArray, "Invalid vertex array handle");
        glBindVertexArray(vertexArray->id);
    }

    void OpenGLGraphicsDevice::BindPipeline(Renderer::PipelineHandle handle)
    {
        const GLPipeline* pipeline = GetPipeline(handle);
        GEF_CORE_ASSERT(pipeline, "Invalid pipeline handle");

        auto shader = GetShader(pipeline->shader);
        GEF_CORE_ASSERT(shader, "Invalid shader handle");

        if (!shader)
            return;

        glUseProgram(shader->program);
        if (pipeline->depth.test)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
        glDepthMask(pipeline->depth.write);

        switch (pipeline->cull)
        {
        case Renderer::CullMode::None:
            glDisable(GL_CULL_FACE);
            break;
        case Renderer::CullMode::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case Renderer::CullMode::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        default:
            glDisable(GL_CULL_FACE);
        }
    }

    void OpenGLGraphicsDevice::BindUniformBuffer(uint32_t bindingPoint,
                                                 Renderer::BufferHandle handle)
    {
        const GLBuffer* buffer = GetBuffer(handle);
        GEF_CORE_ASSERT(buffer, "Invalid buffer handle");
        GEF_CORE_ASSERT(buffer->type == Renderer::BufferType::Uniform,
                        "Buffer is not a uniform buffer");
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, buffer->id);
    }

    void
    OpenGLGraphicsDevice::BindShaderStorageBuffer(uint32_t bindingPoint,
                                                  Renderer::BufferHandle handle)
    {
        const GLBuffer* buffer = GetBuffer(handle);
        GEF_CORE_ASSERT(buffer, "Invalid buffer handle");
        GEF_CORE_ASSERT(buffer->type == Renderer::BufferType::ShaderStorage,
                        "Buffer is not a shader storage buffer");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, buffer->id);
    }

    void OpenGLGraphicsDevice::BindTexture(uint32_t unit,
                                           Renderer::TextureHandle handle)
    {
        const GLTexture* texture =
            textures_.IsValid(handle) ? textures_.Get(handle) : nullptr;
        GEF_CORE_ASSERT(texture, "Invalid texture handle");
        glBindTextureUnit(unit, texture->id);
    }

    void OpenGLGraphicsDevice::DrawIndexedInstanced(uint32_t indexCount,
                                                    uint32_t instanceCount,
                                                    uint32_t baseInstance)
    {
        glDrawElementsInstancedBaseInstance(
            GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT,
            nullptr, static_cast<GLsizei>(instanceCount), baseInstance);
    }

    std::vector<uint8_t>
    OpenGLGraphicsDevice::ReadFramebuffer(uint32_t width, uint32_t height) const
    {
        std::vector<uint8_t> data(width * height * 4);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                     data.data());
        return data;
    }

    void OpenGLGraphicsDevice::SetBackBufferSize(uint32_t width,
                                                 uint32_t height)
    {
        if (width == 0 || height == 0)
            return;

        backBufferWidth_ = width;
        backBufferHeight_ = height;

        // Recreate render target textures
        for (auto const& targetHandle : renderTargets_.GetAliveHandles())
        {
            auto target = renderTargets_.Get(targetHandle);

            if (!target
                || target->desc.sizeMode != Renderer::SizeMode::WindowRelative)
                continue;

            const auto size = ComputeSize(target->desc);
            target->width = size.x;
            target->height = size.y;

            for (uint32_t i = 0; i < Renderer::MaxColorAttachments; ++i)
            {
                auto textureHandle = target->attachments[i];
                if (textureHandle.IsValid() && textures_.IsValid(textureHandle))
                {
                    auto texture = textures_.Get(textureHandle);
                    glDeleteTextures(1, &texture->id);

                    texture->width = size.x;
                    texture->height = size.y;

                    if (!AllocateTextureStorage(*texture))
                    {
                        GEF_CORE_ASSERT(
                            false, "Failed to allocate texture storage buffer");
                        return;
                    }

                    glNamedFramebufferTexture(
                        target->id, GL_COLOR_ATTACHMENT0 + i, texture->id, 0);
                }
            }

            if (target->depthStencil.IsValid()
                && textures_.IsValid(target->depthStencil))
            {
                auto texture = textures_.Get(target->depthStencil);
                glDeleteTextures(1, &texture->id);

                texture->width = size.x;
                texture->height = size.y;

                if (!AllocateTextureStorage(*texture))
                {
                    GEF_CORE_ASSERT(
                        false, "Failed to allocate texture storage buffer");
                    return;
                }

                glNamedFramebufferTexture(target->id, GL_DEPTH_ATTACHMENT,
                                          texture->id, 0);
            }

            GLenum status =
                glCheckNamedFramebufferStatus(target->id, GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
            {
                GEF_ENGINE_ERROR("Render target framebuffer is incomplete");
                DestroyRenderTargetResources(*target);
            }
        }
    }

    void
    OpenGLGraphicsDevice::DestroyRenderTargetResources(GLRenderTarget& target)
    {
        if (target.id != 0)
        {
            glDeleteFramebuffers(1, &target.id);
            target.id = 0;
        }
        for (uint32_t i = 0; i < Renderer::MaxColorAttachments; ++i)
        {
            if (target.attachments[i].IsValid())
            {
                DestroyTexture(target.attachments[i]);
                target.attachments[i] = Renderer::TextureHandle{};
            }
        }
        if (target.depthStencil.IsValid())
        {
            DestroyTexture(target.depthStencil);
            target.depthStencil = Renderer::TextureHandle{};
        }
    }

    bool OpenGLGraphicsDevice::AllocateTextureStorage(GLTexture& texture)
    {
        GLuint id = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, texture.levels, texture.internalFormat,
                           texture.width, texture.height);

        GLint allocatedWidth = 0;
        glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_WIDTH, &allocatedWidth);
        if (allocatedWidth == 0)
        {
            glDeleteTextures(1, &id);
            GEF_ENGINE_ERROR(
                "Failed to allocate immutable texture storage ({}x{})",
                texture.width, texture.height);
            return false;
        }

        bool useMipmaps = texture.levels > 1;
        GLint minFilter = useMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, texture.wrapMode);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, texture.wrapMode);

        texture.id = id;
        return true;
    }

    glm::uvec2 OpenGLGraphicsDevice::ComputeSize(
        const Renderer::RenderTargetDesc& desc) const
    {
        if (desc.sizeMode == Renderer::SizeMode::Fixed)
            return { std::max(desc.width, 1u), std::max(desc.height, 1u) };

        return {
            std::max(1u, static_cast<uint32_t>(backBufferWidth_ * desc.scaleX)),
            std::max(1u, static_cast<uint32_t>(backBufferHeight_ * desc.scaleY))
        };
    }
} // namespace GEF::Platform