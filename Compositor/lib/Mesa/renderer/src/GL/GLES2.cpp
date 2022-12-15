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

#include "../Trace.h"

#include <IBuffer.h>
#include <IRenderer.h>

#include "EGL.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>

#include <core/core.h>

namespace Compositor {
namespace Renderer {
    class GLES : public Interfaces::IRenderer {
        struct Buffer {
            // struct wlr_gles2_renderer* renderer;

            EGLImageKHR image;
            GLuint rbo;
            GLuint fbo;

            WPEFramework::Core::ProxyType<Interfaces::IBuffer> realBuffer;
        };

        static constexpr GLfloat verts[] = {
            1, 0, // top right
            0, 0, // top left
            1, 1, // bottom right
            0, 1, // bottom left
        };

    public:
        GLES() = delete;
        GLES(GLES const&) = delete;
        GLES& operator=(GLES const&) = delete;

        GLES(int fd)
            : _formats()
            , _fd(fd)
            , _egl()
            , _current_buffer(nullptr)
            , _viewport_width(0)
            , _viewport_height(0)
        {
            // Format format(DRM_);
            // _formats.insert();
        }

        virtual ~GLES() = default;

        uint32_t Configure(const string& config)
        {
            return WPEFramework::Core::ERROR_NONE;
        }

        uint32_t Bind(Interfaces::IBuffer* buffer) override
        {
            if (_current_buffer != nullptr) {
                ASSERT(_egl.IsCurrent() == true);

                glFlush();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                _current_buffer->realBuffer->Unlock(100);
                _current_buffer = nullptr;
            }

            if (buffer == nullptr) {
                _egl.ResetCurrent();
                return true;
            }

            _egl.SetCurrent();

            Buffer* glBuffer = dynamic_cast<Buffer*>(buffer);

            if (buffer == NULL) {
                // buffer = create_buffer(renderer, buffer);
            }

            if (buffer == NULL) {
                return false;
            }

            // buffer->realBuffer->Lock(100);

            //_current_buffer = buffer;

            glBindFramebuffer(GL_FRAMEBUFFER, _current_buffer->fbo);

            return true;
        }

        bool Begin(uint32_t width, uint32_t height) override
        {
            // gles2_get_renderer_in_context(wlr_renderer);

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
            //_projection = matrix_projection(width, height, WL_OUTPUT_TRANSFORM_FLIPPED_180);

            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            // XXX: maybe we should save output projection and remove some of the need
            // for users to sling matricies themselves

            // pop_gles2_debug(renderer);

            return true;
        }

        void End() override
        {
            // nothing to do
        }

        void Clear(const Color color) override
        {
            glClearColor(color[0], color[1], color[2], color[3]);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        void Scissor(const Box* box) override
        {
            if (box != nullptr) {
                glScissor(box->x, box->y, box->width, box->height);
                glEnable(GL_SCISSOR_TEST);
            } else {
                glDisable(GL_SCISSOR_TEST);
            }
        }

        uint32_t Render(Interfaces::IBuffer* texture, const Box region, const Matrix transform, float alpha) override
        {
            return 0;
        }

        uint32_t Quadrangle(const Color color, const Matrix transformation) override
        {
            return 0;
        }

        Interfaces::IBuffer* Bound() const
        {
            return nullptr;
        }

        int Handle() const
        {
            return _fd;
        }

        const std::vector<PixelFormat>& RenderFormats() const override
        {
            return _formats;
        }

        const std::vector<PixelFormat>& TextureFormats() const override
        {
            return _formats;
        }

    private:
        bool HasContext() const
        {
            return ((_egl.IsCurrent() == true) && (_current_buffer != nullptr));
        }

    private:
        const std::vector<PixelFormat> _formats;
        int _fd;
        EGL _egl;
        Buffer* _current_buffer;
        uint32_t _viewport_width;
        uint32_t _viewport_height;
        Matrix _projection;
    }; // class GLES
} // namespace Renderer

WPEFramework::Core::ProxyType<Interfaces::IRenderer> Interfaces::IRenderer::Instance(WPEFramework::Core::instance_id identifier)
{
    static WPEFramework::Core::ProxyMapType<WPEFramework::Core::instance_id, Interfaces::IRenderer> glRenderers;

    return glRenderers.Instance<Renderer::GLES>(identifier, static_cast<int>(identifier));
}

} // namespace Compositor
