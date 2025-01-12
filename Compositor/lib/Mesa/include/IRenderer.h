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

            virtual bool IsValid() const = 0;

            virtual uint32_t Width() const = 0;
            virtual uint32_t Height() const = 0;
        }; // struct ITexture

        /**
         * @brief A factory for renderer, callee needs to call Release() when done.
         *
         * @param identifier ID for this Renderer, allows for reuse.
         * @return Core::ProxyType<IRenderer>
         */
        static Core::ProxyType<IRenderer> Instance(Identifier identifier);

        // /**
        //  * @brief Install a callback to receive e.g. the
        //  *
        //  * @param callback A callback pointer, or nullptr to unset the callback.
        //  * @return uint32_t Core::ERROR_NONE upon success, error otherwise.
        //  */
        // virtual uint32_t Callback(ICallback* callback) = 0;

        /**
         * @brief Binds a frame buffer to the renderer, all render related actions will be done using this buffer.
         *
         * @param buffer A preallocated buffer to be used or ```nullptr``` to clear.
         * @return uint32_t Core::ERROR_NONE upon success, error otherwise.
         */
        virtual uint32_t Bind(Core::ProxyType<Exchange::ICompositionBuffer> buffer) = 0;

        /**
         * @brief Clears the active frame buffer from the renderer.
         */
        virtual void Unbind() = 0;

        /**
         * @brief Start a render pass with the provided viewport.
         *
         * This should be called after a binding a buffer, callee must call
         * End() when they are done rendering.
         *
         * @param width Viewport width in pixels
         * @param height Viewport height in pixels
         * @return false on failure, in which case compositors shouldn't try rendering.
         */
        virtual bool Begin(uint32_t width, uint32_t height) = 0;

        /**
         * @brief Ends a render pass.
         *
         */
        virtual void End(bool dump = false) = 0;

        /**
         * @brief Clear the viewport with the provided color
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
        virtual void Scissor(const Exchange::IComposition::Rectangle* box) = 0;

        /**
         * @brief   Creates a texture in the gpu bound to this renderer
         *
         * @param buffer   The buffer representing the pixel data.
         *
         * @return ITexture upon success, nullptr on error.
         */
        virtual Core::ProxyType<ITexture> Texture(const Core::ProxyType<Exchange::ICompositionBuffer>& buffer) = 0;

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
        virtual uint32_t Render(const Core::ProxyType<ITexture>& texture, const Exchange::IComposition::Rectangle& region, const Matrix transform, float alpha) = 0;

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
        virtual uint32_t Quadrangle(const Color color, const Matrix transformation) = 0;

        /**
         * @brief  Returns the buffer currently bound to the renderer
         *
         * @return ICompositionBuffer* or nullptr if no buffer is bound.
         *
         */
        virtual Core::ProxyType<Exchange::ICompositionBuffer> Bound() const = 0;

        /**
         * TODO: We probably want this so we can do screen dumps
         *
         * @brief Reads out of pixels of the currently bound buffer into data.
         *        `stride` is in bytes.
         */
        // virtual uint32_t DumpPixels(uint32_t sourceX, uint32_t sourceY, uint32_t destinationX, uint32_t destinationY, Exchange::ICompositionBuffer* data) = 0;

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
