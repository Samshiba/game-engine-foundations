//
// Created by genin on 10/11/2026.
// Path: Engine/src/Engine/Renderer/Texture.cpp
//

#include <Engine/Renderer/Texture.hpp>

GEF::Renderer::Texture GEF::Renderer::UploadTexture(GraphicsDevice& device,
                                                    const ImageData& data,
                                                    TextureFormat format)
{
    return device.CreateTexture({ .width = data.width,
                                  .height = data.height,
                                  .format = format,
                                  .data = data.pixels.data() });
}

void GEF::Renderer::DestroyTexture(GraphicsDevice& device,
                                   const Texture& texture)
{
    device.DestroyTexture(texture);
}
