// Copyright 2015-2018 Elviss Strazdins. All rights reserved.

#pragma once

#include "core/Setup.h"

#if OUZEL_COMPILE_DIRECT3D11

#include <d3d11.h>
#include "graphics/TextureResource.hpp"

namespace ouzel
{
    namespace graphics
    {
        class RenderDeviceD3D11;

        class TextureResourceD3D11: public TextureResource
        {
        public:
            explicit TextureResourceD3D11(RenderDeviceD3D11& renderDeviceD3D11);
            virtual ~TextureResourceD3D11();

            void init(const std::vector<Texture::Level>& newLevels,
                      uint32_t newFlags = 0,
                      uint32_t newSampleCount = 1,
                      PixelFormat newPixelFormat = PixelFormat::RGBA8_UNORM);

            void setData(const std::vector<Texture::Level>& newLevels);
            void setFilter(Texture::Filter newFilter);
            void setAddressX(Texture::Address newAddressX);
            void setAddressY(Texture::Address newAddressY);
            void setMaxAnisotropy(uint32_t newMaxAnisotropy);
            void setClearColorBuffer(bool clear);
            void setClearDepthBuffer(bool clear);
            void setClearColor(Color color);
            void setClearDepth(float newClearDepth);

            ID3D11Texture2D* getTexture() const { return texture; }
            ID3D11ShaderResourceView* getResourceView() const { return resourceView; }
            ID3D11SamplerState* getSamplerState() const { return samplerState; }

            ID3D11RenderTargetView* getRenderTargetView() const { return renderTargetView; }
            ID3D11Texture2D* getDepthStencilTexture() const { return depthStencilTexture; }
            ID3D11DepthStencilView* getDepthStencilView() const { return depthStencilView; }

            const float* getFrameBufferClearColor() const { return frameBufferClearColor; }

            UINT getWidth() const { return width; }
            UINT getHeight() const { return height; }

            bool getClearFrameBufferView() const { return clearFrameBufferView; }
            bool getClearDepthBufferView() const { return clearDepthBufferView; }

        private:
            void createTexture();
            void updateSamplerState();

            ID3D11Texture2D* texture = nullptr;
            ID3D11ShaderResourceView* resourceView = nullptr;
            ID3D11SamplerState* samplerState = nullptr;

            UINT width = 0;
            UINT height = 0;

            ID3D11RenderTargetView* renderTargetView = nullptr;
            ID3D11Texture2D* depthStencilTexture = nullptr;
            ID3D11DepthStencilView* depthStencilView = nullptr;

            FLOAT frameBufferClearColor[4];
            bool clearFrameBufferView = true;
            bool clearDepthBufferView = false;
        };
    } // namespace graphics
} // namespace ouzel

#endif
