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

#include "../Module.h"

#include <CompositorTypes.h>
#include <IBuffer.h>
#include <interfaces/IComposition.h>
#include <interfaces/ICompositionBuffer.h>
#include <DRM.h>

#include "IOutput.h"
#include "IBackend.h"

#include <wayland-client.h>
#include "generated/presentation-time-client-protocol.h"
#include "generated/xdg-shell-client-protocol.h"

namespace Thunder {
namespace Compositor {
    namespace Backend {
        class WaylandOutput : public IOutput {

            struct PresentationFeedbackEvent {
                PresentationFeedbackEvent() = delete;
                PresentationFeedbackEvent(const PresentationFeedbackEvent& copy) = delete;
                PresentationFeedbackEvent& operator=(const PresentationFeedbackEvent& copy) = delete;
                PresentationFeedbackEvent(PresentationFeedbackEvent&& move) = delete;
                PresentationFeedbackEvent& operator=(PresentationFeedbackEvent&& move) = delete;

                PresentationFeedbackEvent(const uint32_t tv_sec_hi, const uint32_t tv_sec_lo, const uint32_t tv_nsec, const uint32_t refresh_ns, const uint32_t seq_hi, const uint32_t seq_lo, const uint32_t flags, const bool presented)
                    : tv_seconds((static_cast<uint64_t>(tv_sec_hi) << 32) | tv_sec_lo)
                    , tv_nseconds(tv_nsec)
                    , refresh_ns(refresh_ns)
                    , sequence((static_cast<uint64_t>(seq_hi) << 32) | seq_lo)
                    , flags(flags)
                    , presented(presented)
                {
                }

                PresentationFeedbackEvent(const uint64_t sequence)
                    : tv_seconds(0)
                    , tv_nseconds(0)
                    , refresh_ns(0)
                    , sequence(sequence)
                    , flags(0)
                    , presented(true)
                {
                }

                uint64_t tv_seconds;
                uint32_t tv_nseconds;
                uint32_t refresh_ns;
                uint64_t sequence;
                uint32_t flags;
                bool presented;
            }; // struct PresentationFeedbackEvent

        public:
            WaylandOutput() = delete;
            WaylandOutput(WaylandOutput&&) = delete;
            WaylandOutput(const WaylandOutput&) = delete;
            WaylandOutput& operator=(const WaylandOutput&) = delete;
            WaylandOutput(Wayland::IBackend& backend, const string& name, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format);

            virtual ~WaylandOutput();

            Exchange::ICompositionBuffer::IIterator* Acquire(const uint32_t timeoutMs) override;
            void Relinquish() override;

            uint32_t Width() const override;
            uint32_t Height() const override;
            uint32_t Format() const override;
            uint64_t Modifier() const override;
            Exchange::ICompositionBuffer::DataType Type() const override;

            uint32_t Commit() override;
            const string& Node() const override;

            int32_t X() const override
            {
                return _rectangle.x;
            }

            int32_t Y() const override
            {
                return _rectangle.y;
            }

        private:
            static void onSurfaceConfigure(void* data, struct xdg_surface* xdg_surface, uint32_t serial);
            static const struct xdg_surface_listener windowSurfaceListener;

            static void onTopLevelConfigure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states);
            static void onToplevelClose(void* data, struct xdg_toplevel* xdg_toplevel);
            static const struct xdg_toplevel_listener toplevelListener;

            static void onBufferRelease(void* data, struct wl_buffer* buffer);
            static const struct wl_buffer_listener bufferListener;

            static void onPresentationFeedbackSyncOutput(void* data, struct wp_presentation_feedback* feedback, struct wl_output* output);
            static void onPresentationFeedbackPresented(void* data, struct wp_presentation_feedback* feedback, uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns, uint32_t seq_hi, uint32_t seq_lo, uint32_t flags);
            static void onPresentationFeedbackDiscarded(void* data, struct wp_presentation_feedback* feedback);
            static const struct wp_presentation_feedback_listener presentationFeedbackListener;

            void SurfaceConfigure();
            void PresentationFeedback(const PresentationFeedbackEvent& event);
            uint64_t NextSequence()
            {
                return _commitSequence++;
            }

        private:
            Wayland::IBackend& _backend;
            wl_surface* _surface;
            xdg_surface* _windowSurface;
            zxdg_toplevel_decoration_v1* _windowDecoration;
            xdg_toplevel* _topLevelSurface;
            Exchange::IComposition::Rectangle _rectangle;
            uint32_t _format;
            uint64_t _modifier;
            Compositor::Matrix _matrix;
            Core::ProxyType<Exchange::ICompositionBuffer> _buffer;
            Core::Event _signal;
            uint64_t _commitSequence;
        }; // WaylandOutput

    } // namespace Backend
} // namespace Compositor
} // namespace Thunder
