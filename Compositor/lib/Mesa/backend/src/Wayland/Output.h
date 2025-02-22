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
#include "Input.h"

#include <wayland-client.h>
#include "generated/presentation-time-client-protocol.h"
#include "generated/xdg-shell-client-protocol.h"
#include "generated/drm-client-protocol.h"
#include "generated/linux-dmabuf-unstable-v1-client-protocol.h"
#include "generated/pointer-gestures-unstable-v1-client-protocol.h"
#include "generated/presentation-time-client-protocol.h"
#include "generated/relative-pointer-unstable-v1-client-protocol.h"
#include "generated/xdg-activation-v1-client-protocol.h"
#include "generated/xdg-decoration-unstable-v1-client-protocol.h"
#include "generated/xdg-shell-client-protocol.h"

namespace Thunder {
namespace Compositor {
    namespace Backend {
        class WaylandOutput : public IOutput {
        public:
            class Backend : public Core::IResource {
            public:
                typedef struct __attribute__((packed, aligned(4))) {
                    uint32_t format;
                    uint32_t padding;
                    uint64_t modifier;
                } WaylandFormat;

            private:
                using FormatRegister = std::unordered_map<uint32_t, std::vector<uint64_t> >;
                Backend();

            public:
                Backend(Backend&&) = delete;
                Backend(const Backend&) = delete;
                Backend& operator=(Backend&&) = delete;
                Backend& operator=(const Backend&) = delete;

                static Backend& Instance();
                virtual ~Backend();

            public:
                wl_surface* Surface() const;
                xdg_surface* WindowSurface(wl_surface* surface) const;
                int RoundTrip() const;
                int Flush() const;
                void Format(const Compositor::PixelFormat& input, uint32_t& format, uint64_t& modifier) const;
                int RenderNode() const;

                wl_buffer* CreateBuffer(Exchange::ICompositionBuffer* buffer) const;

                struct zxdg_toplevel_decoration_v1* GetWindowDecorationInterface(xdg_toplevel* topLevelSurface) const;
                struct wp_presentation_feedback* GetFeedbackInterface(wl_surface* surface) const;

                void RegisterInterface(struct wl_registry* registry, uint32_t name, const char* iface, uint32_t version);

                void PresentationClock(const uint32_t clockType);
                void HandleDmaFormatTable(int32_t fd, uint32_t size);

                void OpenDrmRender(drmDevice* device);
                void OpenDrmRender(const std::string& name);

                int Dispatch(const uint32_t events) const;

                Core::ProxyType<IOutput> Output(const string& name, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format, Compositor::IOutput::ICallback* feedback);

                handle Descriptor() const override {
                    return (_displayHandle);
                }
                uint16_t Events() override {
                    return (POLLIN | POLLOUT | POLLERR | POLLHUP);
                }
                void Handle(const uint16_t events) override {
                    Dispatch(events);
                }

            private:
                int _drmRenderFd;
                std::string _activationToken;

                wl_display* _wlDisplay;
                wl_registry* _wlRegistry;
                wl_compositor* _wlCompositor;
                wl_drm* _wlDrm;
                clockid_t _presentationClock;
                int _displayHandle;

                xdg_wm_base* _xdgWmBase;
                zxdg_decoration_manager_v1* _wlZxdgDecorationManagerV1;
                zwp_pointer_gestures_v1* _wlZwpPointerGesturesV1;
                wp_presentation* _wlPresentation;
                zwp_linux_dmabuf_v1* _wlZwpLinuxDmabufV1;
                wl_shm* _wlShm;
                zwp_relative_pointer_manager_v1* _wlZwpRelativePointerManagerV1;
                xdg_activation_v1* _wlXdgActivationV1;
                zwp_linux_dmabuf_feedback_v1* _wlZwpLinuxDmabufFeedbackV1;
                FormatRegister _dmaFormats;

                Input _input;

            }; // Backend

        private:
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
            WaylandOutput& operator=(WaylandOutput&&) = delete;
            WaylandOutput& operator=(const WaylandOutput&) = delete;

            WaylandOutput(const string& name, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format);
            ~WaylandOutput() override;

        public:
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
            Backend& _backend;
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
