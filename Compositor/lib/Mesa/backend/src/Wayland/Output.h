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

#include <interfaces/IComposition.h>
#include <interfaces/IGraphicsBuffer.h>

#include <CompositorTypes.h>
#include <IBuffer.h>

#include "IBackend.h"
#include "IOutput.h"

#include <DRM.h>

#ifdef USE_LIBDECOR
#include <libdecor.h>
#endif

#include <wayland-client.h>

#include "generated/presentation-time-client-protocol.h"
#include "generated/xdg-shell-client-protocol.h"

namespace Thunder {
namespace Compositor {
    namespace Backend {
        class VSyncTimer;
        class WaylandOutput : public IOutput {
#ifndef USE_LIBDECOR
            struct PresentationFeedbackEvent {
                PresentationFeedbackEvent() = default;

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
#endif

        public:
            WaylandOutput() = delete;
            WaylandOutput(WaylandOutput&&) = delete;
            WaylandOutput(const WaylandOutput&) = delete;
            WaylandOutput& operator=(const WaylandOutput&) = delete;
            WaylandOutput(Wayland::IBackend& backend, const string& name, const uint32_t width, const uint32_t height, uint32_t refreshRate, const Compositor::PixelFormat& format, const Core::ProxyType<IRenderer>& renderer, Compositor::IOutput::ICallback* feedback);

            virtual ~WaylandOutput();

            // IGraphicsBuffer methods

            PUSH_WARNING(DISABLE_WARNING_OVERLOADED_VIRTUALS)
            Exchange::IGraphicsBuffer::IIterator* Acquire(const uint32_t timeoutMs) override;
            void Relinquish() override;
            POP_WARNING()

            uint32_t Width() const override;
            uint32_t Height() const override;
            uint32_t Format() const override;
            uint64_t Modifier() const override;
            Exchange::IGraphicsBuffer::DataType Type() const override;

            // IOutput methods
            uint32_t Commit() override;
            const string& Node() const override;
            Core::ProxyType<Compositor::IRenderer::IFrameBuffer> FrameBuffer() const override;

            uint32_t WindowWidth() const { return _windowWidth; }
            uint32_t WindowHeight() const { return _windowHeight; }

        private:
            static void onSurfaceConfigure(void* data, struct xdg_surface* xdg_surface, uint32_t serial);
            static const struct xdg_surface_listener windowSurfaceListener;

            static void onBufferRelease(void* data, struct wl_buffer* buffer);
            static const struct wl_buffer_listener bufferListener;

#ifdef USE_LIBDECOR
            static void onLibDecorFrameConfigure(struct libdecor_frame* frame, struct libdecor_configuration* configuration, void* user_data);
            static void onLibDecorFrameClose(struct libdecor_frame* frame, void* user_data);
            static void onLibDecorFrameCommit(struct libdecor_frame* frame, void* user_data);
            static void onLibDecorFrameDismissPopup(struct libdecor_frame* frame, const char* seat_name, void* user_data);
            static struct libdecor_frame_interface libDecorFrameInterface;
#else
            static void onPresentationFeedbackSyncOutput(void* data, struct wp_presentation_feedback* feedback, struct wl_output* output);
            static void onPresentationFeedbackPresented(void* data, struct wp_presentation_feedback* feedback, uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns, uint32_t seq_hi, uint32_t seq_lo, uint32_t flags);
            static void onPresentationFeedbackDiscarded(void* data, struct wp_presentation_feedback* feedback);
            static const struct wp_presentation_feedback_listener presentationFeedbackListener;

            static void onTopLevelConfigure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states);
            static void onToplevelClose(void* data, struct xdg_toplevel* xdg_toplevel);
            static const struct xdg_toplevel_listener toplevelListener;
#endif

#ifdef USE_SURFACE_TRACKING
            static bool IsOurSurface(wl_surface* surface);
            static void onSurfaceEnter(void* data, struct wl_surface* wl_surface, struct wl_output* wl_output);
            static void onSurfaceLeave(void* data, struct wl_surface* wl_surface, struct wl_output* wl_output);
            static const struct wl_surface_listener surfaceListener;
#endif
            void InitializeBuffer();

            void Close();

            void CalculateWindowSize(uint32_t renderWidth, uint32_t renderHeight);
            void HandleWindowResize(uint32_t width, uint32_t height);
            void CorrectAspectRatio(uint32_t& width, uint32_t& height);
            void ConfigureViewportScaling();

#ifdef USE_LIBDECOR
            void CommitLibDecor(libdecor_configuration* pConfiguration);
#else
            void PresentationFeedback(const PresentationFeedbackEvent* event);
#endif
            void SurfaceCommit();

            void WlBufferDestroy(wl_buffer* buffer);
            wl_buffer* WlBufferCreate();

        private:
            Wayland::IBackend& _backend;
            const string _name;
            wl_surface* _surface;
            wp_viewport* _viewport;
            const uint32_t _renderWidth;
            const uint32_t _renderHeight;
            uint32_t _windowWidth;
            uint32_t _windowHeight;
            uint32_t _mfps;
            Compositor::PixelFormat _format;
            Compositor::Matrix _matrix;
            Core::ProxyType<Exchange::IGraphicsBuffer> _buffer;
            Core::Event _signal;
            const Core::ProxyType<Compositor::IRenderer>& _renderer;
            Core::ProxyType<Compositor::IRenderer::IFrameBuffer> _frameBuffer;
            Compositor::IOutput::ICallback* _feedback;
#ifdef USE_LIBDECOR
            libdecor_frame* _libDecorFrame;
            std::atomic<bool> _libDecorCommitNeeded; //
#else
            xdg_surface* _windowSurface;
            xdg_toplevel* _topLevelSurface;
            std::atomic<uint64_t> _sequenceCounter;
            uint64_t _lastPresentationTime;
#endif
            wl_buffer* _wlBuffer;

#ifdef USE_SURFACE_TRACKING
            std::vector<wl_output*> _wlOutputs;
#endif
#ifdef USE_LIBDECOR
            std::unique_ptr<VSyncTimer> _vsyncTimer;
            std::atomic<bool> _commitPending;
#endif
            std::atomic<bool> _terminated;
        }; // WaylandOutput

    } // namespace Backend
} // namespace Compositor
} // namespace Thunder
