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

#include <Module.h>

#include <IRenderer.h>
#include <Trace.h>

#include "EGL.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>

using namespace WPEFramework;

namespace Compositor {
namespace Renderer {

    class GLES : virtual public Interfaces::IRender {
        struct Buffer : virtual public Interfaces::IBuffer {
            // struct wlr_gles2_renderer* renderer;

            EGLImageKHR image;
            GLuint rbo;
            GLuint fbo;

            // struct wlr_addon addon;
        };

        static constexpr GLfloat verts[] = {
            1, 0, // top right
            0, 0, // top left
            1, 1, // bottom right
            0, 1, // bottom left
        };

    public:
        GLES()
            : _formats()
            , _fd(-1)
            , _egl()
            , _current_buffer(nullptr)
            , _viewport_width(0)
            , _viewport_height(0)
        {
            // Format format(DRM_);
            // _formats.insert();
        }

        virtual ~GLES() = default;

        GLES(GLES const&) = delete;
        GLES& operator=(GLES const&) = delete;

        uint32_t Initialize(const string& config)
        {
        }

        uint32_t Deinitialize()
        {
        }

        bool Bind(IBuffer* buffer)
        {
            if (_current_buffer != nullptr) {
                ASSERT(_egl.IsCurrent() == true);

                glFlush();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                _current_buffer->Unlock();
                _current_buffer = nullptr;
            }

            if (buffer == nullptr) {
                _egl.ResetCurrent();
                return true;
            }

            _egl.SetCurrent();

            Buffer* glBuffer = dynamic_cast<Buffer*>(buffer);

            if (buffer == NULL) {
                buffer = create_buffer(renderer, buffer);
            }

            if (buffer == NULL) {
                return false;
            }

            wlr_buffer_lock(buffer);

            renderer->current_buffer = buffer;

            glBindFramebuffer(GL_FRAMEBUFFER, renderer->current_buffer->fbo);

            return true;
        }

        bool Begin(uint32_t width, uint32_t height)
        {
            gles2_get_renderer_in_context(wlr_renderer);

            // if (renderer->procs.glGetGraphicsResetStatusKHR) {
            //     GLenum status = renderer->procs.glGetGraphicsResetStatusKHR();
            //     if (status != GL_NO_ERROR) {
            //         wlr_log(WLR_ERROR, "GPU reset (%s)", reset_status_str(status));
            //         wl_signal_emit_mutable(&wlr_renderer->events.lost, NULL);
            //         return false;
            //     }
            // }

            glViewport(0, 0, width, height);

            _viewport_width = width;
            _viewport_height = height;

            // refresh projection matrix
            _projection = matrix_projection(width, height, WL_OUTPUT_TRANSFORM_FLIPPED_180);

            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            // XXX: maybe we should save output projection and remove some of the need
            // for users to sling matricies themselves

            // pop_gles2_debug(renderer);

            return true;
        }

        void End()
        {
            // nothing to do
        }

        void Clear(const Color color)
        {
            glClearColor(color[0], color[1], color[2], color[3]);
            glClear(GL_COLOR_BUFFER_BIT)
        }

        void Scissor(const Box* box)
        {
            if (box != nullptr) {
                glScissor(box->x, box->y, box->width, box->height);
                glEnable(GL_SCISSOR_TEST);
            } else {
                glDisable(GL_SCISSOR_TEST);
            }
        }

        bool RenderTexture(ITexture* texture, const Box region, const Matrix transform, float alpha)
        {
        }

        void RenderQuad(const Box region, const Matrix transform)
        {
        }

        int Handle() const
        {
            return _fd;
        }

        uint32_t BufferCapabilities() const
        {
            return IBuffer::Capability::DMABUF;
        }

        // ITexture* ToTexture(Interfaces::IBuffer* buffer);

        const std::list<Format>& RenderFormats() const
        {
            return _formats;
        }

        const std::list<Format>& TextureFormats() const
        {
            return _formats;
        }

    private:
        void HasContext() const
        {
            return (_egl.IsCurrent()) && (_current_buffer != nullptr);
        }

        Core::ProxyType<Buffer> CreateBuffer()
        {
        }

    private:
        const std::list<Format> _formats;
        int _fd;
        EGL _egl;
        Buffer* _current_buffer;
        uint32_t _viewport_width;
        uint32_t _viewport_height;
        IRenderer::Matrix _projection;

    }; // class GLES

    static IRenderer* Create(const Exchange::IComposition* compositor)
    {
        static GLES implementation;

        return implementation;
    }
} // namespace Renderer
} // namespace Compositor
