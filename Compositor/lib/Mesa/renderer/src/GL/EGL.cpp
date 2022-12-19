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

#ifndef MODULE_NAME
#define MODULE_NAME CompositorRendererEGL
#endif

#include "EGL.h"

#include <tracing/tracing.h>

#include <DrmCommon.h>

#include <gbm.h>

MODULE_NAME_ARCHIVE_DECLARATION

namespace {
namespace Trace {
    class EGL {
    public:
        ~EGL() = default;
        EGL() = delete;
        EGL(const EGL&) = delete;
        EGL& operator=(const EGL&) = delete;
        EGL(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            WPEFramework::Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit EGL(const string& text)
            : _text(WPEFramework::Core::ToString(text))
        {
        }

    public:
        const char* Data() const
        {
            return (_text.c_str());
        }
        uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    }; // class EGL
}

void DebugSink(EGLenum error, const char* command, EGLint messageType, EGLLabelKHR /*threadLabel*/, EGLLabelKHR /*objectLabel*/, const char* message)
{
    std::stringstream line;
    line << "command: " << command
         << ", result: " << Compositor::API::EGL::ErrorString(error)
         /*<< ", thread: \"" << threadLabel*/
         /*<< ", object: \"" << objectLabel */
         << ", message: \"" << message << "\"";

    switch (messageType) {
    case EGL_DEBUG_MSG_CRITICAL_KHR: {
        TRACE_GLOBAL(WPEFramework::Trace::Fatal, ("%s", line.str().c_str()));
        break;
    }
    case EGL_DEBUG_MSG_ERROR_KHR: {
        TRACE_GLOBAL(WPEFramework::Trace::Error, ("%s", line.str().c_str()));
        break;
    }
    case EGL_DEBUG_MSG_WARN_KHR: {
        TRACE_GLOBAL(WPEFramework::Trace::Warning, ("%s", line.str().c_str()));
        break;
    }
    case EGL_DEBUG_MSG_INFO_KHR: {
        TRACE_GLOBAL(WPEFramework::Trace::Information, ("%s", line.str().c_str()));
        break;
    }
    default: {
        TRACE_GLOBAL(Trace::EGL, ("%s", line.str().c_str()));
        break;
    }
    }
}

string ConfigInfoLog(EGLDisplay dpy, EGLConfig config)
{
    std::stringstream stream;

    EGLint id, bufferSize, redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize;

    eglGetConfigAttrib(dpy, config, EGL_CONFIG_ID, &id);
    eglGetConfigAttrib(dpy, config, EGL_BUFFER_SIZE, &bufferSize);
    eglGetConfigAttrib(dpy, config, EGL_RED_SIZE, &redSize);
    eglGetConfigAttrib(dpy, config, EGL_GREEN_SIZE, &greenSize);
    eglGetConfigAttrib(dpy, config, EGL_BLUE_SIZE, &blueSize);
    eglGetConfigAttrib(dpy, config, EGL_ALPHA_SIZE, &alphaSize);
    eglGetConfigAttrib(dpy, config, EGL_DEPTH_SIZE, &depthSize);
    eglGetConfigAttrib(dpy, config, EGL_STENCIL_SIZE, &stencilSize);

    stream << "id=" << id << "  -- buffer=" << bufferSize << "  -- red=" << redSize << "  -- green=" << greenSize << "  -- blue=" << blueSize << "  -- alpha=" << alphaSize << "  -- depth=" << depthSize << "  -- stencil=" << stencilSize;

    return stream.str();
}

std::vector<EGLConfig> MatchConfigs(EGLDisplay dpy, const EGLint attrib_list[])
{
    EGLint count(0);

    if (eglGetConfigs(dpy, nullptr, 0, &count) != EGL_TRUE) {
        TRACE_GLOBAL(WPEFramework::Trace::Error, ("Unable to get EGL configs 0x%x", eglGetError()));
    }

    TRACE_GLOBAL(Trace::EGL, ("Got a total of %d EGL configs.", count));

    std::vector<EGLConfig> configs(count, nullptr);

    EGLint matches(0);

    if (eglChooseConfig(dpy, attrib_list, configs.data(), configs.size(), &matches) != EGL_TRUE) {
        TRACE_GLOBAL(WPEFramework::Trace::Error, ("No EGL configs with appropriate attributes: 0x%x", eglGetError()));
    }

    TRACE_GLOBAL(Trace::EGL, ("Found %d matching EGL configs", matches));

    configs.erase(std::remove_if(configs.begin(), configs.end(),
                      [](EGLConfig& c) { return (c == nullptr); }),
        configs.end());

    uint8_t i(0);

    for (auto cnf : configs) {
        TRACE_GLOBAL(Trace::EGL, ("%d. %s", i++, ConfigInfoLog(dpy, cnf).c_str()));
    }

    return configs;
}
}

namespace Compositor {
namespace Renderer {
    EGL::EGL(const int drmFd)
        : _api()
        , _display(EGL_NO_DISPLAY)
        , _context(EGL_NO_CONTEXT)
    {
        constexpr EGLAttrib debugAttributes[] = {
            EGL_DEBUG_MSG_CRITICAL_KHR,
            EGL_TRUE, //
            EGL_DEBUG_MSG_ERROR_KHR,
            EGL_TRUE, //
            EGL_DEBUG_MSG_WARN_KHR,
            EGL_TRUE, //
            EGL_DEBUG_MSG_INFO_KHR,
            EGL_TRUE, //
            EGL_NONE,
        };

        ASSERT(_api.eglDebugMessageControl != nullptr);
        _api.eglDebugMessageControl(DebugSink, debugAttributes);

        EGLBoolean eglBind = eglBindAPI(EGL_OPENGL_ES_API);

        ASSERT(eglBind == EGL_TRUE);

        bool isMaster = drmIsMaster(drmFd);

        EGLDeviceEXT eglDevice = FindEGLDevice(drmFd);

        if (eglDevice != EGL_NO_DEVICE_EXT) {
            InitializeEgl(EGL_PLATFORM_DEVICE_EXT, eglDevice, isMaster);
        } else {
            int gbm_fd = DRM::ReopenNode(drmFd, true);

            gbm_device* gbmDevice = gbm_create_device(gbm_fd);

            if (gbmDevice != nullptr) {
                InitializeEgl(EGL_PLATFORM_GBM_KHR, gbmDevice, isMaster);
                gbm_device_destroy(gbmDevice);
            }

            close(gbm_fd);
        }

        ASSERT(_display != EGL_NO_DISPLAY);
        ASSERT(_context != EGL_NO_CONTEXT);
    }

    EGL::~EGL()
    {
        ASSERT(_display != EGL_NO_DISPLAY);

        eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (_context != EGL_NO_CONTEXT) {
            eglDestroyContext(_display, _context);
            _context = EGL_NO_CONTEXT;
        }

        eglTerminate(_display);

        eglReleaseThread();
    }

    EGLDeviceEXT EGL::FindEGLDevice(const int drmFd)
    {
        EGLDeviceEXT result(EGL_NO_DEVICE_EXT);

        if ((_api.eglQueryDevicesEXT != nullptr) && (_api.eglQueryDeviceStringEXT != nullptr)) {
            EGLint nEglDevices = 0;
            _api.eglQueryDevicesEXT(0, nullptr, &nEglDevices);

            EGLDeviceEXT eglDevices[nEglDevices];

            _api.eglQueryDevicesEXT(nEglDevices, eglDevices, &nEglDevices);

            if (nEglDevices > 0) {
                drmDevice* drmDevice = nullptr;
                int ret = drmGetDevice(drmFd, &drmDevice);

                if (drmDevice != nullptr) {
                    for (int i = 0; i < nEglDevices; i++) {
                        const char* deviceName = _api.eglQueryDeviceStringEXT(eglDevices[i], EGL_DRM_DEVICE_FILE_EXT);

                        TRACE(Trace::EGL, ("Found EGL device %s", deviceName));

                        bool nodePresent = DRM::HasNode(drmDevice, deviceName);

                        if ((deviceName != nullptr) && (nodePresent == true)) {
                            TRACE(Trace::EGL, ("Using EGL device %s", deviceName));
                            result = eglDevices[i];
                            break;
                        }
                    }

                    drmFreeDevice(&drmDevice);
                } else {
                    TRACE(WPEFramework::Trace::Error, ("No DRM devices found"));
                }
            } else {
                TRACE(WPEFramework::Trace::Error, ("No EGL devices found"));
            }
        } else {
            TRACE(WPEFramework::Trace::Error, ("EGL device enumeration not supported"));
        }

        return result;
    }

    uint32_t EGL::InitializeEgl(EGLenum platform, void* remote_display, bool isMaster)
    {
        uint32_t result(WPEFramework::Core::ERROR_NONE);

        API::Attributes<EGLint> displayAttributes;

        if (API::EGL::HasExtension("EGL_KHR_display_reference") == true) {
            displayAttributes.append(EGL_TRACK_REFERENCES_KHR, EGL_TRUE);
        }

        ASSERT(_api.eglGetPlatformDisplayEXT != nullptr);

        _display = _api.eglGetPlatformDisplayEXT(platform, remote_display, displayAttributes);

        if (_display != EGL_NO_DISPLAY) {
            EGLint major, minor;

            if (eglInitialize(_display, &major, &minor) == EGL_TRUE) {
                std::vector<EGLint> contextAttributes;

                contextAttributes.push_back(EGL_CONTEXT_CLIENT_VERSION);
                contextAttributes.push_back(2);

                if ((isMaster == true) && (API::EGL::HasExtension("EGL_IMG_context_priority") == true)) {
                    contextAttributes.push_back(EGL_CONTEXT_PRIORITY_LEVEL_IMG);
                    contextAttributes.push_back(EGL_CONTEXT_PRIORITY_HIGH_IMG);
                    TRACE(Trace::EGL, ("Using high context priority"));
                }

                if (API::EGL::HasExtension("EGL_EXT_create_context_robustness") == true) {
                    contextAttributes.push_back(EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT);
                    contextAttributes.push_back(EGL_LOSE_CONTEXT_ON_RESET_EXT);
                    TRACE(Trace::EGL, ("Using context robustness"));
                }

                contextAttributes.push_back(EGL_NONE);

                _context = eglCreateContext(_display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, contextAttributes.data());

                if (_context != EGL_NO_CONTEXT) {
                    if ((isMaster == true) && (API::EGL::HasExtension("EGL_IMG_context_priority") == true)) {
                        EGLint priority = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;

                        eglQueryContext(_display, _context, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &priority);

                        if (priority != EGL_CONTEXT_PRIORITY_HIGH_IMG) {
                            TRACE(WPEFramework::Trace::Error, ("Failed to obtain a high priority context"));
                        } else {
                            TRACE(Trace::EGL, ("Obtained high priority context"));
                        }
                    }
                } else {
                    TRACE(WPEFramework::Trace::Error, ("eglCreateContext failed: %s", API::EGL::ErrorString(eglGetError())));
                    result = WPEFramework::Core::ERROR_UNAVAILABLE;
                }
            } else {
                TRACE(WPEFramework::Trace::Error, ("eglInitialize failed: %s", API::EGL::ErrorString(eglGetError())));
                result = WPEFramework::Core::ERROR_UNAVAILABLE;
            }
        } else {
            TRACE(WPEFramework::Trace::Error, ("eglGetPlatformDisplayEXT failed: %s", API::EGL::ErrorString(eglGetError())));
            result = WPEFramework::Core::ERROR_UNAVAILABLE;
        }

        return result;
    }

    std::vector<PixelFormat> EGL::Formats()
    {
        return std::vector<PixelFormat>();
    }

} // namespace Renderer
} // namespace Compositor