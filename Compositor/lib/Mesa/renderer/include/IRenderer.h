/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "IBuffer.h"
#include "PixelFormat.h"
#include <interfaces/IComposition.h>

using namespace WPEFramework;

namespace Compositor {
namespace Interfaces {

    template <const uin32_t TYPE>
    class ITexture : public IBuffer<TYPE> {
    };

    struct IRenderer {
        using Box = Exchange::IComposition::Rectangle;
        using Matrix = float matrix[9];
        using Color = float color[4];

        enum class Type : uint8_t {
            Pixman = 0x01, // Generic (Software)
            Gles2 = 0x02,  // Hardware
            Vulkan = 0x03, // Hardware
        };

        virtual ~IRenderer() = default;

        static IRenderer* Create(const Exchange::IComposition* compositor);

        virtual uint32_t Initialize(const string& config) = 0;
        virtual uint32_t Deinitialize() = 0;

        virtual bool Bind(Interfaces::IBuffer* buffer) = 0;

        /**
         * @brief
         *
         * @param width
         * @param height
         * @return true
         * @return false
         */
        virtual bool Begin(uint32_t width, uint32_t height) = 0;
        virtual void End() = 0;

        /**
         * @brief Renders a solid colour.
         *
         * @param color
         */
        virtual void Clear(const Color color) = 0;

        /**
         * @brief Scissor defines a rectangle, called the scissor box, in window coordinates.
         *        If used only pixels that lie within the scissor box can be modified by drawing commands.
         *        Window coordinates have integer values at the shared corners of frame buffer pixels.
         *
         * @param box a box describing the region to write or nullptr to disable
         */
        virtual void Scissor(const Box* box) = 0;

        /**
         * @brief Renders a texture on a surface.
         *
         * @param texture           Texture object to render
         * @param region            The coordinates and size where to render.
         * @param transformation    A transformation matrix
         * @param alpha             The opacity of the render
         *
         * @return uint32_t Core::ERROR_NONE if all went ok, error code otherwise.
         */
        virtual uint32_t RenderTexture(ITexture* texture, const Box region, const Matrix transform, float alpha) = 0;

        /**
         * @brief Renderer a
         *
         * @param region            The coordinates and size where to render.
         * @param transformation    A transformation matrix
         *
         * @return uint32_t Core::ERROR_NONE if all went ok, error code otherwise.
         */
        virtual uint32_t RenderQuad(const Box region, const Matrix transformation) = 0;

        virtual int Handle() const = 0;

        /**
         * @brief Get a list of supported buffer types
         *
         * @return uint32_t
         */
        // virtual std::list<uint32_t> BufferCapabilities() const = 0;

        /**
         * @brief Returns a list of pixel @PixelFormat valid for rendering.
         *
         * @return const std::vector<PixelFormat>& the list of @Formats
         */
        virtual const std::vector<PixelFormat>& RenderFormats() const = 0;

        /**
         * @brief Returns a list of pixel @PixelFormat valid for textures.
         *
         * @return const std::vector<PixelFormat>& the list of @Formats
         */
        virtual const std::vector<PixelFormat>& TextureFormats() const = 0;

        /**
         * @brief
         *
         * @param buffer
         * @return ITexture*
         */
        // static ITexture* ToTexture(Interfaces::IBuffer* buffer)
        // {
        //     return dynamic_cast<ITexture*>(buffer);
        // }
    }; // struct IRenderer
}
} // namespace Compositor