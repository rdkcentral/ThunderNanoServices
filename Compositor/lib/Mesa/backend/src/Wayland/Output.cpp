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
#include <vector>

#include <CompositorUtils.h>

#include <thread>
#include <chrono>

namespace Thunder {
namespace Compositor {
    namespace Backend {
        static const char* AppId = "com.thunder.compositor";

        class VSyncTimer {
        public:
            VSyncTimer(uint32_t mFps)
                : _frameInterval(std::chrono::microseconds(1000000000 / mFps))
                , _sequenceCounter(0)
                , _running(false)
                , _timerThread()
                , _callback(nullptr)
            {
            }

            ~VSyncTimer()
            {
                Stop();
            }

            void Start(std::function<void(uint64_t sequence, uint64_t timestamp_us)> presented_callback)
            {
                if (_running)
                    return;

                _callback = presented_callback;
                _running = true;
                _timerThread = std::thread(&VSyncTimer::TimerLoop, this);
            }

            void Stop()
            {
                if (_running) {
                    _running = false;
                    if (_timerThread.joinable()) {
                        _timerThread.join();
                    }
                }
            }

        private:
            void TimerLoop()
            {
                auto next_vsync = std::chrono::high_resolution_clock::now() + _frameInterval;

                while (_running) {
                    // Sleep until next "vsync"
                    std::this_thread::sleep_until(next_vsync);

                    if (!_running)
                        break;

                    // Generate timestamp
                    auto now = std::chrono::high_resolution_clock::now();
                    auto timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        now.time_since_epoch())
                                            .count();

                    // Call the presented callback - this triggers your existing vsync handling!
                    if (_callback) {
                        _callback(++_sequenceCounter, timestamp_us);
                    }

                    // Calculate next vsync time
                    next_vsync += _frameInterval;

                    // Catch up if we're behind
                    if (next_vsync < now) {
                        next_vsync = now + _frameInterval;
                    }
                }
            }

        private:
            std::chrono::microseconds _frameInterval;
            std::atomic<uint64_t> _sequenceCounter;
            std::atomic<bool> _running;
            std::thread _timerThread;
            std::function<void(uint64_t, uint64_t)> _callback;
        };

        void WaylandOutput::onSurfaceConfigure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
        {
            xdg_surface_ack_configure(xdg_surface, serial);
            TRACE_GLOBAL(Trace::Backend, ("Acknowledged Surface Configure"));

            WaylandOutput* implementation = static_cast<WaylandOutput*>(data);
            implementation->InitializeBuffer();
        }

        const struct xdg_surface_listener WaylandOutput::windowSurfaceListener = {
            .configure = onSurfaceConfigure,
        };

        void WaylandOutput::onBufferRelease(void* data, struct wl_buffer* buffer)
        {
            WaylandOutput* implementation = static_cast<WaylandOutput*>(data);
            implementation->WlBufferDestroy(buffer);
        }

        const struct wl_buffer_listener WaylandOutput::bufferListener = {
            .release = onBufferRelease,
        };

#ifdef USE_SURFACE_TRACKING
        bool WaylandOutput::IsOurSurface(wl_surface* surface)
        {
            return (surface != nullptr) && (wl_proxy_get_tag((wl_proxy*)surface) == &AppId);
        }

        void WaylandOutput::onSurfaceEnter(void* data, struct wl_surface* wl_surface, struct wl_output* wl_output)
        {
            TRACE_GLOBAL(Trace::Backend, ("Surface Entered Output"));

            if (WaylandOutput::IsOurSurface(wl_surface) == true) {
                WaylandOutput* implementation = static_cast<WaylandOutput*>(data);

                implementation->_wlOutputs.emplace_back(wl_output);
            } else {
                TRACE_GLOBAL(Trace::Backend, ("Surface is not ours, ignoring enter event"));
            }
        }

        void WaylandOutput::onSurfaceLeave(void* data, struct wl_surface* wl_surface, struct wl_output* wl_output)
        {
            TRACE_GLOBAL(Trace::Backend, ("Surface Left Output"));

            if (WaylandOutput::IsOurSurface(wl_surface) == true) {
                WaylandOutput* implementation = static_cast<WaylandOutput*>(data);
                implementation->_wlOutputs.erase(std::remove(implementation->_wlOutputs.begin(), implementation->_wlOutputs.end(), wl_output), implementation->_wlOutputs.end());
            } else {
                TRACE_GLOBAL(Trace::Backend, ("Surface is not ours, ignoring enter event"));
            }
        }

        const struct wl_surface_listener WaylandOutput::surfaceListener = {
            .enter = onSurfaceEnter,
            .leave = onSurfaceLeave,
            .preferred_buffer_scale = nullptr,
            .preferred_buffer_transform = nullptr,
        };
#endif

#ifdef USE_LIBDECOR
        void WaylandOutput::onLibDecorFrameConfigure(struct libdecor_frame* frame, struct libdecor_configuration* configuration, void* userData)
        {
            WaylandOutput* implementation = static_cast<WaylandOutput*>(userData);

            if (frame == implementation->_libDecorFrame) {
                int width(0), height(0);

                if (libdecor_configuration_get_content_size(configuration, frame, &width, &height)) {
                    const float renderAspect = static_cast<float>(implementation->_renderWidth) / static_cast<float>(implementation->_renderHeight);

                    int correctedWidth = width;
                    int correctedHeight = static_cast<int>(std::round(width / renderAspect));

                    if (correctedHeight > height) {
                        correctedHeight = height;
                        correctedWidth = static_cast<int>(std::round(height * renderAspect));
                    }

                    implementation->HandleWindowResize(correctedWidth, correctedHeight);

                    struct libdecor_state* state = libdecor_state_new(correctedWidth, correctedHeight);
                    libdecor_frame_commit(frame, state, configuration);
                    libdecor_state_free(state);
                }

                implementation->InitializeBuffer();
            }
        }

        void WaylandOutput::onLibDecorFrameClose(struct libdecor_frame*, void* userData)
        {
            WaylandOutput* implementation = static_cast<WaylandOutput*>(userData);
            implementation->Close();
        }

        void WaylandOutput::onLibDecorFrameCommit(struct libdecor_frame*, void* userData)
        {
            WaylandOutput* implementation = static_cast<WaylandOutput*>(userData);

            implementation->Commit();

            // bool expected = false;

            // if (implementation->_libDecorCommitNeeded.compare_exchange_strong(expected, true) == true) {
            //     TRACE_GLOBAL(Trace::Information, ("LibDecor Frame Commit requested"));
            // }
        }

        void WaylandOutput::onLibDecorFrameDismissPopup(struct libdecor_frame*, const char*, void*)
        {
            // not used
        }

        struct libdecor_frame_interface WaylandOutput::libDecorFrameInterface = {
            .configure = onLibDecorFrameConfigure,
            .close = onLibDecorFrameClose,
            .commit = onLibDecorFrameCommit,
            .dismiss_popup = onLibDecorFrameDismissPopup
        };
#else
        void WaylandOutput::onPresentationFeedbackSyncOutput(void* data, struct wp_presentation_feedback* feedback, struct wl_output* output)
        {
            if (feedback != nullptr) {
                wp_presentation_feedback_destroy(feedback);
            }
        }

        void WaylandOutput::onPresentationFeedbackPresented(void* data, struct wp_presentation_feedback* feedback, uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns, uint32_t seq_hi, uint32_t seq_lo, uint32_t flags)
        {
            WaylandOutput* implementation = static_cast<WaylandOutput*>(data);

            const WaylandOutput::PresentationFeedbackEvent event(tv_sec_hi, tv_sec_lo, tv_nsec, refresh_ns, seq_hi, seq_lo, flags, true);

            implementation->PresentationFeedback(&event);

            if (feedback != nullptr) {
                wp_presentation_feedback_destroy(feedback);
            }
        }

        void WaylandOutput::onPresentationFeedbackDiscarded(void* data, struct wp_presentation_feedback* feedback)
        {
            WaylandOutput* implementation = static_cast<WaylandOutput*>(data);

            implementation->PresentationFeedback(nullptr);

            if (feedback != nullptr) {
                wp_presentation_feedback_destroy(feedback);
            }
        }

        const struct wp_presentation_feedback_listener WaylandOutput::presentationFeedbackListener = {
            .sync_output = onPresentationFeedbackSyncOutput,
            .presented = onPresentationFeedbackPresented,
            .discarded = onPresentationFeedbackDiscarded,
        };

        void WaylandOutput::onTopLevelConfigure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states)
        {
            TRACE_GLOBAL(Trace::Backend, ("Toplevel Configure window width: %d height: %d", width, height));

            if (width == 0 || height == 0) {
                // Compositor is asking us to choose the size - use our calculated window size
                return;
            }

            WaylandOutput* implementation = static_cast<WaylandOutput*>(data);

            implementation->HandleWindowResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        }

        void WaylandOutput::onToplevelClose(void* data, struct xdg_toplevel* xdg_toplevel)
        {
            TRACE_GLOBAL(Trace::Backend, ("Bye bye, cruel world! <(x_X)>"));
            WaylandOutput* implementation = static_cast<WaylandOutput*>(data);
            implementation->Close();
        }

        const struct xdg_toplevel_listener WaylandOutput::toplevelListener = {
            .configure = onTopLevelConfigure,
            .close = onToplevelClose,
        };
#endif

        WaylandOutput::WaylandOutput(
            Wayland::IBackend& backend, const string& name,
            const uint32_t width, const uint32_t height, uint32_t refreshRate, const Compositor::PixelFormat& format,
            const Core::ProxyType<IRenderer>& renderer, Compositor::IOutput::ICallback* feedback)
            : _backend(backend)
            , _name(name)
            , _surface(nullptr)
            , _viewport(nullptr)
            , _renderWidth(width)
            , _renderHeight(height)
            , _windowWidth(0)
            , _windowHeight(0)
            , _mfps(refreshRate) // milli frames per second
            , _format(format)
            , _buffer()
            , _signal(false, true)
            , _renderer(renderer)
            , _frameBuffer()
            , _feedback(feedback)
#ifdef USE_LIBDECOR
            , _libDecorFrame(nullptr)
            , _libDecorCommitNeeded(false)
#else
            , _windowSurface(nullptr)
            , _topLevelSurface(nullptr)
            , _sequenceCounter(0)
            , _lastPresentationTime(0)
#endif
            , _wlBuffer(nullptr)
#ifdef USE_SURFACE_TRACKING
            , _wlOutputs()
#endif
            , _vsyncTimer(nullptr)
        {
            TRACE(Trace::Backend, ("Constructing wayland output for '%s'", name.c_str()));

            CalculateWindowSize(_renderWidth, _renderHeight);

            if (format.IsValid() == false) {
                TRACE(Trace::Information, (_T("Selecting format... ")));

                TRACE(Trace::Backend, (_T("Renderer exposes %d render formats. "), renderer->RenderFormats().size()));
                TRACE(Trace::Backend, (_T("Backend accepts %d formats."), backend.Formats().size()));

                _format = Compositor::IntersectFormat(format, { &renderer->RenderFormats(), &backend.Formats() });

                TRACE(Trace::Information, ("Selected format: %s", Compositor::ToString(_format).c_str()));

                ASSERT(_format.IsValid() == true);
            }

            _surface = _backend.Surface(&AppId);
            ASSERT(_surface != nullptr);

            const char* const* appId = wl_proxy_get_tag(reinterpret_cast<wl_proxy*>(_surface));

#ifdef USE_SURFACE_TRACKING
            wl_surface_set_user_data(_surface, this);
            wl_surface_add_listener(_surface, &surfaceListener, this);
#endif

#ifdef USE_LIBDECOR
            _libDecorFrame = libdecor_decorate(backend.LibDecorContext(), _surface, &libDecorFrameInterface, this);
            ASSERT(_libDecorFrame != nullptr);

            struct libdecor_state* frameState = libdecor_state_new(_windowWidth, _windowHeight);
            libdecor_frame_commit(_libDecorFrame, frameState, nullptr);
            libdecor_state_free(frameState);

            libdecor_frame_set_title(_libDecorFrame, name.c_str());
            libdecor_frame_set_app_id(_libDecorFrame, *appId);
            libdecor_frame_map(_libDecorFrame);
            libdecor_frame_set_visibility(_libDecorFrame, true);

#else
            _windowSurface = _backend.WindowSurface(_surface);
            ASSERT(_windowSurface != nullptr);

            _topLevelSurface = xdg_surface_get_toplevel(_windowSurface);
            ASSERT(_topLevelSurface != nullptr);

            xdg_toplevel_set_title(_topLevelSurface, name.c_str());
            xdg_toplevel_set_app_id(_topLevelSurface, *appId);

            xdg_toplevel_add_listener(_topLevelSurface, &toplevelListener, this);
            xdg_surface_add_listener(_windowSurface, &windowSurfaceListener, this);
#endif

            _viewport = _backend.Viewport(_surface);
            ASSERT(_viewport != nullptr);

            wl_surface_commit(_surface);
            _backend.RoundTrip();

            _signal.Lock(1000); // Wait for the surface and buffer to be configured

#ifdef USE_LIBDECOR
            // Only start the vsync timer when we have a window managed by libdecor
            // it somehow it breaks the presentation feedback loop.
            if (_libDecorFrame != nullptr) {
                _vsyncTimer = new VSyncTimer(refreshRate > 0 ? refreshRate : 60);

                _vsyncTimer->Start([this](uint64_t sequence, uint64_t timestamp_us) {
                    if (_feedback != nullptr) {
                        _feedback->Presented(this, sequence, timestamp_us);
                    }
                });
            }
#endif
        }

        WaylandOutput::~WaylandOutput()
        {
            TRACE(Trace::Backend, ("Destructing wayland output for '%s'", _name.c_str()));

            if (_vsyncTimer != nullptr) {
                _vsyncTimer->Stop();
                delete _vsyncTimer;
                _vsyncTimer = nullptr;
            }

            _signal.ResetEvent();

#ifdef USE_SURFACE_TRACKING
            _wlOutputs.clear();
#endif

            if (_viewport != nullptr) {
                wp_viewport_destroy(_viewport);
                _viewport = nullptr;
            }

            if (_wlBuffer != nullptr) {
                wl_buffer_destroy(_wlBuffer);
                _wlBuffer = nullptr;
            }

#ifdef USE_LIBDECOR
            if (_libDecorFrame != nullptr) {
                libdecor_frame* frame = _libDecorFrame;
                _libDecorFrame = nullptr;

                libdecor_frame_unref(frame);
            }
#else
            if (_topLevelSurface != nullptr) {
                xdg_toplevel_destroy(_topLevelSurface);
                _topLevelSurface = nullptr;
            }

            if (_windowSurface != nullptr) {
                xdg_surface_destroy(_windowSurface);
                _windowSurface = nullptr;
            }
#endif

            if (_surface != nullptr) {
                wl_surface_destroy(_surface);
                _surface = nullptr;
            }

            if (_frameBuffer.IsValid() == true) {
                _frameBuffer.Release();
            }
        }

        Exchange::IGraphicsBuffer::IIterator* WaylandOutput::Acquire(const uint32_t timeoutMs)
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

        Exchange::IGraphicsBuffer::DataType WaylandOutput::Type() const
        {
            return (_buffer.IsValid() == true) ? _buffer->Type() : Exchange::IGraphicsBuffer::DataType::TYPE_INVALID;
        }

        void WaylandOutput::InitializeBuffer()
        {
            ASSERT(_surface != nullptr);
            ASSERT(_backend.RenderNode() > 0);

            if (_buffer.IsValid() == false) {
                TRACE(Trace::Information, ("Create buffer with format: %s", Compositor::ToString(_format).c_str()));
                _buffer = Compositor::CreateBuffer(_backend.RenderNode(), _renderWidth, _renderHeight, _format);
                ASSERT(_buffer.IsValid() == true);

                if (_renderer.IsValid() == true) {
                    _frameBuffer = _renderer->FrameBuffer(_buffer);
                    ASSERT(_frameBuffer.IsValid() == true);
                }

                _wlBuffer = _backend.Buffer(_buffer.operator->());
                ASSERT(_wlBuffer != nullptr);

                wl_surface_attach(_surface, _wlBuffer, 0, 0);

                ConfigureViewportScaling();

                _signal.SetEvent();
            }
        }

        uint32_t WaylandOutput::Commit()
        {
            ASSERT(_surface != nullptr);

            if ((_buffer.IsValid() == true)) {

#ifndef USE_LIBDECOR
                struct wp_presentation_feedback* feedback = _backend.PresentationFeedback(_surface);
                wp_presentation_feedback_add_listener(feedback, &presentationFeedbackListener, this);
#endif

                // wl_buffer* buffer = WlBufferCreate();
                // wl_surface_attach(_surface, buffer, 0, 0);

                wl_surface_damage(_surface, 0, 0, INT32_MAX, INT32_MAX);
                wl_surface_set_buffer_scale(_surface, 1);
            } else {
                wl_surface_attach(_surface, nullptr, 0, 0);
                wl_surface_damage(_surface, 0, 0, INT32_MAX, INT32_MAX);
            }

            SurfaceCommit();

            return (Core::ERROR_NONE);
        }
#ifdef USE_LIBDECOR
        void WaylandOutput::CommitLibDecor(libdecor_configuration* configuration)
        {
            if (_libDecorFrame != nullptr) {
                struct libdecor_state* state = libdecor_state_new(_windowWidth, _windowHeight);
                libdecor_frame_commit(_libDecorFrame, state, configuration);
                libdecor_state_free(state);
            }
        }
#endif

        void WaylandOutput::SurfaceCommit()
        {
            if (_surface != nullptr) {
#ifdef USE_LIBDECOR
                bool expected = true;
                if (_libDecorCommitNeeded.compare_exchange_strong(expected, false) == true) {
                    CommitLibDecor(nullptr);
                }
#endif
                wl_surface_commit(_surface);
            }
        }

        const string& WaylandOutput::Node() const /* override */
        {
            return _name;
        }

        Core::ProxyType<Compositor::IRenderer::IFrameBuffer> WaylandOutput::FrameBuffer() const /* override */
        {
            return _frameBuffer;
        }

#ifndef USE_LIBDECOR
        void WaylandOutput::PresentationFeedback(const PresentationFeedbackEvent* event)
        {
            static auto lastFrameTime = std::chrono::high_resolution_clock::now();

            uint64_t currentPts;
            bool shouldThrottle = false;

            if (event != nullptr) {
                struct timespec presentationTimestamp;
                presentationTimestamp.tv_sec = event->tv_seconds;
                presentationTimestamp.tv_nsec = event->tv_nseconds;

                currentPts = Core::Time(presentationTimestamp).Ticks();
                _sequenceCounter = event->sequence;
            } else {
                // Feedback was discarded or came too fast - throttle this
                shouldThrottle = true;
                uint64_t now = Core::Time::Now().Ticks();
                ASSERT(now > _lastPresentationTime);
                _sequenceCounter++;
                currentPts = now;
            }

            if (_mfps > 0) {
                const auto frameInterval = std::chrono::microseconds(1000000000 / _mfps); // Convert milli-fps to microseconds

                // Check if we're running faster than target fps
                auto now = std::chrono::high_resolution_clock::now();
                auto timeSinceLastFrame = now - lastFrameTime;

                if (shouldThrottle || timeSinceLastFrame < frameInterval) {
                    auto sleepDuration = frameInterval - timeSinceLastFrame;

                    if (sleepDuration > std::chrono::microseconds(0)) {
                        std::this_thread::sleep_for(sleepDuration);

                        // Update timing after sleep
                        now = std::chrono::high_resolution_clock::now();
                        currentPts = Core::Time::Now().Ticks();
                    }
                }

                lastFrameTime = now;
            }

            if (_feedback != nullptr) {
                _feedback->Presented(this, _sequenceCounter, currentPts);
            }

            _lastPresentationTime = currentPts;
        }
#endif

        void WaylandOutput::Close()
        {
            if (_feedback != nullptr) {
                _feedback->Terminated(this);
            }
        }

        void WaylandOutput::CalculateWindowSize(uint32_t renderWidth, uint32_t renderHeight)
        {
            // Target window sizes for development comfort
            constexpr uint32_t MAX_WINDOW_WIDTH = 1280;
            constexpr uint32_t MAX_WINDOW_HEIGHT = 720;
            constexpr uint32_t MIN_WINDOW_WIDTH = 640;
            constexpr uint32_t MIN_WINDOW_HEIGHT = 480;

            // Calculate render aspect ratio
            const float renderAspect = static_cast<float>(renderWidth) / static_cast<float>(renderHeight);

            // If render size fits within max bounds, use it as-is to preserve exact aspect ratio
            if (renderWidth <= MAX_WINDOW_WIDTH && renderHeight <= MAX_WINDOW_HEIGHT) {
                _windowWidth = renderWidth;
                _windowHeight = renderHeight;
                TRACE(Trace::Backend, ("Using exact render size %ux%u (aspect: %.3f)", renderWidth, renderHeight, renderAspect));
                return;
            }

            // Scale down while preserving aspect ratio
            uint32_t targetWidth, targetHeight;

            // Try scaling by width constraint
            targetWidth = MAX_WINDOW_WIDTH;
            targetHeight = static_cast<uint32_t>(targetWidth / renderAspect);

            // If height exceeds limit, scale by height constraint instead
            if (targetHeight > MAX_WINDOW_HEIGHT) {
                targetHeight = MAX_WINDOW_HEIGHT;
                targetWidth = static_cast<uint32_t>(targetHeight * renderAspect);
            }

            // Ensure minimum size while trying to preserve aspect ratio
            if (targetWidth < MIN_WINDOW_WIDTH) {
                targetWidth = MIN_WINDOW_WIDTH;
                targetHeight = static_cast<uint32_t>(targetWidth / renderAspect);
            }
            if (targetHeight < MIN_WINDOW_HEIGHT) {
                targetHeight = MIN_WINDOW_HEIGHT;
                targetWidth = static_cast<uint32_t>(targetHeight * renderAspect);
            }

            _windowWidth = targetWidth;
            _windowHeight = targetHeight;

            const float actualAspect = static_cast<float>(_windowWidth) / static_cast<float>(_windowHeight);
            const float aspectError = std::abs(actualAspect - renderAspect) / renderAspect * 100.0f;

            TRACE(Trace::Backend, ("Scaled window: %ux%u (aspect: %.3f, error: %.1f%%) from render %ux%u", _windowWidth, _windowHeight, actualAspect, aspectError, renderWidth, renderHeight));
        }

        void WaylandOutput::HandleWindowResize(uint32_t newWidth, uint32_t newHeight)
        {
            if (newWidth == _windowWidth && newHeight == _windowHeight) {
                return; // No change
            }

            TRACE(Trace::Backend, ("Window resize: %ux%u → %ux%u (render buffer stays %ux%u)", _windowWidth, _windowHeight, newWidth, newHeight, _renderWidth, _renderHeight));

            _windowWidth = newWidth;
            _windowHeight = newHeight;

            ConfigureViewportScaling();
        }

        void WaylandOutput::ConfigureViewportScaling()
        {
            if (_viewport != nullptr) {
                // Calculate aspect ratios
                const float renderAspect = static_cast<float>(_renderWidth) / static_cast<float>(_renderHeight);
                const float windowAspect = static_cast<float>(_windowWidth) / static_cast<float>(_windowHeight);

                if (std::abs(renderAspect - windowAspect) < 0.001f) {
                    // Aspects match - use full viewport
                    wp_viewport_set_source(_viewport,
                        wl_fixed_from_int(0), wl_fixed_from_int(0),
                        wl_fixed_from_int(_renderWidth), wl_fixed_from_int(_renderHeight));
                    wp_viewport_set_destination(_viewport, _windowWidth, _windowHeight);
                } else {
                    // Preserve aspect ratio with letterboxing/pillarboxing
                    uint32_t destWidth, destHeight;

                    if (renderAspect > windowAspect) {
                        // Render is wider - fit to width, letterbox height
                        destWidth = _windowWidth;
                        destHeight = static_cast<uint32_t>(_windowWidth / renderAspect);
                    } else {
                        // Render is taller - fit to height, pillarbox width
                        destHeight = _windowHeight;
                        destWidth = static_cast<uint32_t>(_windowHeight * renderAspect);
                    }

                    wp_viewport_set_source(_viewport,
                        wl_fixed_from_int(0), wl_fixed_from_int(0),
                        wl_fixed_from_int(_renderWidth), wl_fixed_from_int(_renderHeight));
                    wp_viewport_set_destination(_viewport, destWidth, destHeight);
                }

                TRACE(Trace::Backend, ("Viewport: render %ux%u (%.3f) → window %ux%u (%.3f)", _renderWidth, _renderHeight, renderAspect, _windowWidth, _windowHeight, windowAspect));
            }
        }

        void WaylandOutput::WlBufferDestroy(wl_buffer* buffer)
        {
            TRACE(Trace::Information, ("Destroy Wayland buffer"));
            wl_buffer_destroy(buffer);
        }

        wl_buffer* WaylandOutput::WlBufferCreate()
        {
            wl_buffer* buffer(nullptr);

            if (_buffer.IsValid() == true) {
                TRACE(Trace::Information, ("Create Wayland buffer"));

                buffer = _backend.Buffer(_buffer.operator->());
                wl_buffer_add_listener(buffer, &bufferListener, this);
            }

            return buffer;
        }
    } // namespace Backend
} // namespace Compositor
} // namespace Thunder
