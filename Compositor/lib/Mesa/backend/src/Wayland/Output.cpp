/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological B.V.
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

#include <Output.h>

#include <drm_fourcc.h>

namespace Thunder {
namespace Compositor {
    namespace Backend {

        /**
         * @brief Callback for the configure event on the surface
         *
         * @param data User data passed to the callback
         * @param xdg_surface The xdg_surface object
         * @param serial The serial of the configure event
         *
         */
        void WaylandOutput::onSurfaceConfigure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
        {
            xdg_surface_ack_configure(xdg_surface, serial);
            TRACE_GLOBAL(Trace::Backend, ("Acknowledged Surface Configure"));

            static_cast<WaylandOutput*>(data)->SurfaceConfigure();
        }

        const struct xdg_surface_listener WaylandOutput::windowSurfaceListener = {
            .configure = onSurfaceConfigure,
        };

        /**
         * @brief Callback for the configure event on the toplevel
         *
         * @param data User data passed to the callback
         * @param xdg_toplevel The xdg_toplevel object
         * @param width The new width of the toplevel
         * @param height The new height of the toplevel
         * @param states The states of the toplevel
         *
         */
        void WaylandOutput::onTopLevelConfigure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states)
        {
            TRACE_GLOBAL(Trace::Backend, ("Toplevel Configure width: %d height: %d", width, height));

            // if (width == 0 || height == 0) {
            //     return;
            // }

            // WaylandOutput* implementation = static_cast<WaylandOutput*>(data);

            // implementation->_width = width;
            // implementation->_height = height;
        }

        /**
         * @brief Callback for the close event on the toplevel
         *
         * @param data User data passed to the callback
         * @param xdg_toplevel The xdg_toplevel object
         *
         */
        void WaylandOutput::onToplevelClose(void* data, struct xdg_toplevel* xdg_toplevel)
        {
            TRACE_GLOBAL(Trace::Backend, ("Bye bye, cruel world! <(x_X)>"));
            WaylandOutput* implementation = static_cast<WaylandOutput*>(data);
            // Maybe we want to delete ?
            // implementation->Release();
        }

        const struct xdg_toplevel_listener WaylandOutput::toplevelListener = {
            .configure = onTopLevelConfigure,
            .close = onToplevelClose,
        };

        /**
         * @brief Callback for the release event on the buffer
         *
         * @param data User data passed to the callback
         * @param buffer The wl_buffer object
         *
         */
        void WaylandOutput::onBufferRelease(void* data, struct wl_buffer* buffer)
        {
            /* Sent by the compositor when it's no longer using this buffer */
            wl_buffer_destroy(buffer);
        }

        const struct wl_buffer_listener WaylandOutput::bufferListener = {
            .release = onBufferRelease,
        };

        void WaylandOutput::onPresentationFeedbackSyncOutput(void* data, struct wp_presentation_feedback* feedback, struct wl_output* output)
        {
            if (feedback != nullptr) {
                wp_presentation_feedback_destroy(feedback);
            }
        }

        void WaylandOutput::onPresentationFeedbackPresented(void* data, struct wp_presentation_feedback* feedback, uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns, uint32_t seq_hi, uint32_t seq_lo, uint32_t flags)
        {
            WaylandOutput* implementation = static_cast<WaylandOutput*>(data);
            implementation->PresentationFeedback(WaylandOutput::PresentationFeedbackEvent(tv_sec_hi, tv_sec_lo, tv_nsec, refresh_ns, seq_hi, seq_lo, flags, true));

            if (feedback != nullptr) {
                wp_presentation_feedback_destroy(feedback);
            }
        }

        void WaylandOutput::onPresentationFeedbackDiscarded(void* data, struct wp_presentation_feedback* feedback)
        {
            WaylandOutput* implementation = static_cast<WaylandOutput*>(data);

            if (feedback != nullptr) {
                wp_presentation_feedback_destroy(feedback);
            }
        }

        const struct wp_presentation_feedback_listener WaylandOutput::presentationFeedbackListener = {
            .sync_output = onPresentationFeedbackSyncOutput,
            .presented = onPresentationFeedbackPresented,
            .discarded = onPresentationFeedbackDiscarded,
        };

        WaylandOutput::WaylandOutput(
            Wayland::IBackend& backend, const string& name,
            const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format)
            : _backend(backend)
            , _surface(nullptr)
            , _windowSurface(nullptr)
            , _windowDecoration(nullptr)
            , _topLevelSurface(nullptr)
            , _rectangle(rectangle)
            , _format()
            , _modifier()
            , _buffer()
            , _signal(false, true)
            , _commitSequence(0)
        {
            TRACE(Trace::Backend, ("Constructing wayland output for '%s'", name.c_str()));

            _backend.Format(format, _format, _modifier);
            TRACE(Trace::Backend, ("Picked DMA format: %s modifier: 0x%" PRIX64, DRM::FormatToString(_format), _modifier));

            ASSERT(_format != DRM_FORMAT_INVALID);
            ASSERT(_modifier != DRM_FORMAT_MOD_INVALID);

            if (Compositor::Rectangle::IsDefault(rectangle)) {
                _rectangle.width = 1280;
                _rectangle.height = 720;
            }

            _surface = _backend.Surface();

            ASSERT(_surface != nullptr);

            wl_surface_set_user_data(_surface, this);

            _windowSurface = _backend.WindowSurface(_surface);

            ASSERT(_windowSurface != nullptr);

            _topLevelSurface = xdg_surface_get_toplevel(_windowSurface);

            ASSERT(_topLevelSurface != nullptr);

            _windowDecoration = _backend.GetWindowDecorationInterface(_topLevelSurface);

            if (_windowDecoration != nullptr) {
                zxdg_toplevel_decoration_v1_set_mode(_windowDecoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
            }

            xdg_toplevel_set_title(_topLevelSurface, name.c_str());

            _backend.Flush();

            xdg_toplevel_set_app_id(_topLevelSurface, name.c_str());

            xdg_surface_add_listener(_windowSurface, &windowSurfaceListener, this);
            xdg_toplevel_add_listener(_topLevelSurface, &toplevelListener, this);

            _backend.RoundTrip();

            wl_surface_commit(_surface);

            // _backend.RoundTrip();

            _signal.Lock(1000); // Wait for the surface and buffer to be configured
        }

        WaylandOutput::~WaylandOutput()
        {
            _signal.ResetEvent();

            if (_windowDecoration != nullptr) {
                zxdg_toplevel_decoration_v1_destroy(_windowDecoration);
            }

            if (_topLevelSurface != nullptr) {
                xdg_toplevel_destroy(_topLevelSurface);
            }

            if (_windowSurface != nullptr) {
                xdg_surface_destroy(_windowSurface);
            }

            if (_surface != nullptr) {
                wl_surface_destroy(_surface);
            }
        }

        Exchange::ICompositionBuffer::IIterator* WaylandOutput::Acquire(const uint32_t timeoutMs)
        {
            return (_buffer.IsValid() == true) ? _buffer->Acquire(timeoutMs) : nullptr;
        }

        void WaylandOutput::Relinquish()
        {
            if (_buffer.IsValid() == true) {
                _buffer->Relinquish();
            }
        }

        uint32_t WaylandOutput::Width() const
        {
            return (_buffer.IsValid() == true) ? _buffer->Width() : 0;
        }

        uint32_t WaylandOutput::Height() const
        {
            return (_buffer.IsValid() == true) ? _buffer->Height() : 0;
        }

        uint32_t WaylandOutput::Format() const
        {
            return (_buffer.IsValid() == true) ? _buffer->Format() : 0;
        }

        uint64_t WaylandOutput::Modifier() const
        {
            return (_buffer.IsValid() == true) ? _buffer->Modifier() : 0;
        }

        Exchange::ICompositionBuffer::DataType WaylandOutput::Type() const
        {
            return (_buffer.IsValid() == true) ? _buffer->Type() : Exchange::ICompositionBuffer::DataType::TYPE_INVALID;
        }

        void WaylandOutput::SurfaceConfigure() {

            ASSERT(_surface != nullptr);
            ASSERT(_backend.RenderNode() > 0);

            if (_buffer.IsValid() == false) {
                _buffer = Compositor::CreateBuffer(_backend.RenderNode(), _rectangle.width, _rectangle.height, Compositor::PixelFormat(_format, { _modifier }));

                wl_buffer* buffer = _backend.CreateBuffer(_buffer.operator->());

                wl_buffer_add_listener(buffer, &bufferListener, nullptr);

                wl_surface_attach(_surface, buffer, 0, 0);

                _signal.SetEvent();
            }

            Commit();

        }
        uint32_t WaylandOutput::Commit()
        {
            ASSERT ((_surface != nullptr) && (_buffer.IsValid() == true));

            wl_surface_commit(_surface);

            _backend.Flush();

            return (Core::ERROR_NONE);
        }

        const string& WaylandOutput::Node() const /* override */
        {
            static string result("TODO");
            return result;
        }

        void WaylandOutput::PresentationFeedback(const PresentationFeedbackEvent& event)
        {
            // TODO: Implement presentation feedback handling here.
        }
    } // namespace Backend
} // namespace Compositor
} // namespace Thunder
