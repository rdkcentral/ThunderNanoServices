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

#include <CompositorTypes.h>

namespace Thunder {
namespace Compositor {
    struct EXTERNAL IRenderer {

        virtual ~IRenderer() = default;

        struct ITexture {
            virtual ~ITexture() = default;

            virtual uint32_t Width() const = 0;
            virtual uint32_t Height() const = 0;

            virtual unsigned int Id() const = 0;

            virtual uint32_t Draw(const float& alpha, const Matrix& matrix, const Exchange::IComposition::Rectangle& region) const = 0;

        }; // struct ITexture

        /**
         * @brief A factory for renderer, callee needs to call Release() when done.
         *
         * @param identifier ID for this Renderer, allows for reuse.
         * @return Core::ProxyType<IRenderer>
         */
        static Core::ProxyType<IRenderer> Instance(const int identifier);

        /**
         * @brief   Creates a texture in the gpu bound to this buffer
         *
         * @param buffer   The buffer representing the data to be associated with context.
         *
         * @return ITexture upon success, nullptr on error.
         */
        virtual Core::ProxyType<ITexture> Texture(const Core::ProxyType<Exchange::ICompositionBuffer>& buffer) = 0;

        /**
         * @brief Set a viewport.
         *
         * @param width Viewport width in pixels
         * @param height Viewport height in pixels
         * @return Core::ERROR_UNAVAILABLE on failure, in which case compositors shouldn't try rendering.
         */
        virtual uint32_t ViewPort(const uint32_t width, const uint32_t height) = 0;

        /**
         * @brief Clear the viewport with the provided color
         *
         * @param color
         */
        virtual void Clear(const Core::ProxyType<ITexture>& image, const Color color) = 0;

        /**
         * @brief   Renders a texture on the bound buffer at the given region with
         *          transforming and transparency info.
         *
         * @param texture           Texture to be rendered
         * @param region            The coordinates and size where to render.
         * @param transformation    A transformation matrix
         * @param alpha             The opacity of the render
         *
         *
         * @return uint32_t Core::ERROR_NONE if all went ok, error code otherwise.
         */
        virtual uint32_t Render(const Core::ProxyType<ITexture>& image, const Core::ProxyType<ITexture>& texture, const Exchange::IComposition::Rectangle& region, const Matrix transform, float alpha) = 0;

        /**
         * @brief Scissor defines a rectangle, called the scissor box, in window coordinates.
         *        If used only pixels that lie within the scissor box can be modified by drawing commands.
         *        Window coordinates have integer values at the shared corners of frame buffer pixels.
         *
         * @param box a box describing the region to write or nullptr to disable
         */
        virtual void Scissor(const Core::ProxyType<ITexture>& image, const Exchange::IComposition::Rectangle* box) = 0;

        /**
         * @brief   Renders a solid quadrangle* in the specified color with the specified matrix.
         *
         * @param region            The coordinates and size where to render.
         * @param transformation    A transformation matrix
         *
         * @return uint32_t Core::ERROR_NONE if all went ok, error code otherwise.
         *
         *  * A geometric shape with four angles and four straight sides;
         *    a four-sided polygon.
         *
         */
        virtual uint32_t Quadrangle(const Core::ProxyType<ITexture>& image, const Color color, const Matrix transformation) = 0;

        /**
         * @brief Returns a list of pixel @PixelFormat valid for rendering.
         *
         * @return const std::vector<PixelFormat>& the list of @PixelFormat
         */
        virtual const std::vector<PixelFormat>& RenderFormats() const = 0;

        /**
         * @brief Returns a list of pixel @PixelFormat valid for textures.
         *
         * @return const std::vector<PixelFormat>& the list of @PixelFormat
         */
        virtual const std::vector<PixelFormat>& TextureFormats() const = 0;

        /**
         * @brief Returns the current projection matrix of the renderer
         *
         * @return const Matrix& the projection matrix currently used
         */
        virtual const Matrix& Projection() const = 0;

    }; // struct EXTERNAL IRenderer

} // namespace Compositor

} // namespace Thunder
