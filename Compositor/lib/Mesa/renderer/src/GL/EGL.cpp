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

#include "../Module.h"
#include "EGL.h"

#include <messaging/messaging.h>
#include <DRM.h>
#include <drm.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <iomanip>

MODULE_NAME_ARCHIVE_DECLARATION

namespace Thunder {

#ifdef __DEBUG__
void DebugSink(EGLenum error, const char* command, EGLint messageType, EGLLabelKHR /*threadLabel*/, EGLLabelKHR /*objectLabel*/, const char* message)
{
    std::stringstream line;
    line << command << "="
         << Compositor::API::EGL::ErrorString(error)
         /*<< ", thread: \"" << threadLabel*/
         /*<< ", object: \"" << objectLabel */
         << "  \"" << message << "\"";

    switch (messageType) {
    case EGL_DEBUG_MSG_CRITICAL_KHR: {
        TRACE_GLOBAL(Trace::Fatal, ("%s", line.str().c_str()));
        break;
    }
    case EGL_DEBUG_MSG_ERROR_KHR: {
        TRACE_GLOBAL(Trace::Error, ("%s", line.str().c_str()));
        break;
    }
    case EGL_DEBUG_MSG_WARN_KHR: {
        TRACE_GLOBAL(Trace::Warning, ("%s", line.str().c_str()));
        break;
    }
    case EGL_DEBUG_MSG_INFO_KHR: {
        TRACE_GLOBAL(Trace::Information, ("%s", line.str().c_str()));
        break;
    }
    default: {
        TRACE_GLOBAL(Trace::EGL, ("%s", line.str().c_str()));
        break;
    }
    }
}
#endif

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
        TRACE_GLOBAL(Trace::Error, ("Unable to get EGL configs 0x%x", eglGetError()));
    }

    TRACE_GLOBAL(Trace::EGL, ("Got a total of %d EGL configs.", count));

    std::vector<EGLConfig> configs(count, nullptr);

    EGLint matches(0);

    if (eglChooseConfig(dpy, attrib_list, configs.data(), configs.size(), &matches) != EGL_TRUE) {
        TRACE_GLOBAL(Trace::Error, ("No EGL configs with appropriate attributes: 0x%x", eglGetError()));
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

namespace Compositor {
    namespace Renderer {
        constexpr int InvalidFileDescriptor = -1;

        EGL::EGL(const int drmFd)
            : _api()
            , _display(EGL_NO_DISPLAY)
            , _context(EGL_NO_CONTEXT)
            , _device(EGL_NO_DEVICE_EXT)
            , _draw_surface(EGL_NO_SURFACE)
            , _read_surface(EGL_NO_SURFACE)
            , _formats()
            , _gbmDescriptor(InvalidFileDescriptor)
            , _gbmDevice(nullptr)
        {
            TRACE(Trace::EGL, ("%s - build: %s", __func__, __TIMESTAMP__));
#ifdef __DEBUG__
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

#endif

            EGLBoolean eglBind = eglBindAPI(EGL_OPENGL_ES_API);

            ASSERT(eglBind == EGL_TRUE);

            const bool isMaster = drmIsMaster(drmFd);

            EGLDeviceEXT eglDevice = FindEGLDevice(drmFd);

            if (eglDevice != EGL_NO_DEVICE_EXT) {
                TRACE(Trace::EGL, ("Initialize EGL using EGL device."));
                Initialize(EGL_PLATFORM_DEVICE_EXT, eglDevice, isMaster);
            } else {
                _gbmDescriptor = DRM::ReopenNode(drmFd, true);
                _gbmDevice = gbm_create_device(_gbmDescriptor);

                if (_gbmDevice != nullptr) {
                    TRACE(Trace::EGL, ("Initialize EGL using GMB device."));
                    Initialize(EGL_PLATFORM_GBM_KHR, _gbmDevice, isMaster);
                }

                ASSERT(_gbmDevice != nullptr);
            }

            GetPixelFormats(_formats);

            TRACE(Trace::EGL, ("Initialized EGL Display=%p, Context=%p, %d formats supported", _display, _context, _formats.size()));
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

            if (_gbmDevice != nullptr) {
                gbm_device_destroy(_gbmDevice);
            }
            if (_gbmDescriptor != InvalidFileDescriptor) {
                close(_gbmDescriptor);
            }

            TRACE(Trace::EGL, ("EGL instance %p destructed", this));
        }

        EGLDeviceEXT EGL::FindEGLDevice(const int drmFd)
        {
            EGLDeviceEXT result(EGL_NO_DEVICE_EXT);

            if ((_api.eglQueryDevicesEXT != nullptr) && (_api.eglQueryDeviceStringEXT != nullptr)) {
                EGLint nEglDevices = 0;
                _api.eglQueryDevicesEXT(0, nullptr, &nEglDevices);
                TRACE(Trace::EGL, ("System has %u EGL device%s", nEglDevices, (nEglDevices == 1) ? "" : "s"));

                EGLDeviceEXT* eglDevices = static_cast<EGLDeviceEXT*>(ALLOCA(nEglDevices * sizeof(EGLDeviceEXT)));

                memset(eglDevices, 0, sizeof(nEglDevices * sizeof(EGLDeviceEXT)));

                _api.eglQueryDevicesEXT(nEglDevices, eglDevices, &nEglDevices);

                if (nEglDevices > 0) {
                    drmDevice* drmDevice = nullptr;
                    int ret = drmGetDevice(drmFd, &drmDevice);

                    if ((ret == 0) && (drmDevice != nullptr)) {
                        for (int i = 0; i < nEglDevices; i++) {
                            TRACE(Trace::EGL, ("Trying device %d [%p]", i, eglDevices[i]));

#ifdef __DEBUG__
                            const char* extensions = _api.eglQueryDeviceStringEXT(eglDevices[i], EGL_EXTENSIONS);
                            if (extensions != nullptr) {
                                TRACE(Trace::EGL, ("EGL extensions: %s", extensions));
                            }
#endif
                            const char* renderName = _api.eglQueryDeviceStringEXT(eglDevices[i], EGL_DRM_RENDER_NODE_FILE_EXT);

                            if (renderName != nullptr) {
                                TRACE(Trace::EGL, ("EGL render node: %s", renderName));
                            } else {
                                TRACE(Trace::Error, ("No EGL render node associated to %p", eglDevices[i]));
                            }

                            const char* deviceName = _api.eglQueryDeviceStringEXT(eglDevices[i], EGL_DRM_DEVICE_FILE_EXT);
                            bool nodePresent(false);

                            if (deviceName == nullptr) {
                                TRACE(Trace::Error, ("No EGL device name returned for %p", eglDevices[i]));
                                continue;
                            } else {
                                TRACE(Trace::EGL, ("Found EGL device %s", deviceName));

                                for (uint16_t i = 0; i < DRM_NODE_MAX; i++) {
                                    if ((drmDevice->available_nodes & (1 << i)) && (strcmp(drmDevice->nodes[i], deviceName) == 0)) {
                                        nodePresent = true;
                                        break;
                                    }
                                }
                            }

                            if ((deviceName != nullptr) && (nodePresent == true)) {
                                TRACE(Trace::EGL, ("Using EGL device %s", deviceName));
                                result = eglDevices[i];
                                break;
                            }
                        }

                        drmFreeDevice(&drmDevice);
                    } else {
                        TRACE(Trace::Error, ("No DRM devices found"));
                    }
                } else {
                    TRACE(Trace::Error, ("No EGL devices found"));
                }
            } else {
                TRACE(Trace::Error, ("EGL device enumeration not supported"));
            }

            return result;
        }

        uint32_t EGL::Initialize(EGLenum platform, void* remote_display, const bool isMaster)
        {
            uint32_t result(Core::ERROR_NONE);

            API::Attributes<EGLint> displayAttributes;

            const std::string clientExtensions(eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));

            TRACE(Trace::EGL, ("Client extensions: %s", clientExtensions.c_str()));

            if (API::HasExtension(clientExtensions, "EGL_KHR_display_reference") == true) {
                displayAttributes.Append(EGL_TRACK_REFERENCES_KHR, EGL_TRUE);
            }

            ASSERT(_api.eglGetPlatformDisplayEXT != nullptr);

            _display = _api.eglGetPlatformDisplayEXT(platform, remote_display, displayAttributes);

            ASSERT(_display != EGL_NO_DISPLAY);

            EGLint major, minor;

            if (eglInitialize(_display, &major, &minor) == EGL_TRUE) {
                TRACE(Trace::EGL, ("EGL Initialized v%d.%d", major, minor));

                API::Attributes<EGLint> contextAttributes;

                contextAttributes.Append(EGL_CONTEXT_CLIENT_VERSION, 2);

                const std::string displayExtensions(eglQueryString(_display, EGL_EXTENSIONS));

                TRACE(Trace::EGL, ("Display extensions: %s", displayExtensions.c_str()));

                if ((isMaster == true) && (API::HasExtension(displayExtensions, "EGL_IMG_context_priority") == true)) {
                    contextAttributes.Append(EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_HIGH_IMG);
                    TRACE(Trace::EGL, ("Using high context priority"));
                }

                if (API::HasExtension(displayExtensions, "EGL_EXT_create_context_robustness") == true) {
                    contextAttributes.Append(EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT, EGL_LOSE_CONTEXT_ON_RESET_EXT);
                    TRACE(Trace::EGL, ("Using context robustness"));
                }

                ASSERT(API::HasExtension(displayExtensions, "EGL_KHR_no_config_context") == true);
                ASSERT(API::HasExtension(displayExtensions, "EGL_MESA_configless_context") == true);
                ASSERT(API::HasExtension(displayExtensions, "EGL_KHR_surfaceless_context") == true);

                if ((_api.eglQueryDisplayAttribEXT != nullptr) && (_api.eglQueryDeviceStringEXT != nullptr)) {
                    // EGLAttrib device_attrib;

                    _api.eglQueryDisplayAttribEXT(_display, EGL_DEVICE_EXT, reinterpret_cast<EGLAttrib*>(&_device));

                    const string deviceExtensions(_api.eglQueryDeviceStringEXT(_device, EGL_EXTENSIONS));

                    TRACE(Trace::EGL, ("Device extensions: %s", deviceExtensions.c_str()));

                    if (API::HasExtension(deviceExtensions, "EGL_MESA_device_software")) {
                        TRACE(Trace::EGL, ("Software rendering detected"));
                    }

                    TRACE(Trace::EGL, ("Vendor: %s", eglQueryString(_display, EGL_VENDOR)));

                    if (API::HasExtension(deviceExtensions, "EGL_EXT_device_persistent_id")) {
                        const char* driverName = _api.eglQueryDeviceStringEXT(_device, EGL_DRIVER_NAME_EXT);
                        TRACE(Trace::EGL, ("Driver: %s", driverName));
                    }
                }

                _context = eglCreateContext(_display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, contextAttributes);
                ASSERT(_context != EGL_NO_CONTEXT);

                if ((isMaster == true) && (API::HasExtension(displayExtensions, "EGL_IMG_context_priority") == true)) {
                    EGLint priority = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;

                    eglQueryContext(_display, _context, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &priority);

                    if (priority != EGL_CONTEXT_PRIORITY_HIGH_IMG) {
                        TRACE(Trace::Error, ("Failed to obtain a high priority context"));
                    } else {
                        TRACE(Trace::EGL, ("Obtained high priority context"));
                    }
                }
            } else {
                TRACE(Trace::Error, ("eglInitialize failed: %s", API::EGL::ErrorString(eglGetError())));
                result = Core::ERROR_UNAVAILABLE;
            }

            return result;
        }

        const std::vector<PixelFormat>& EGL::Formats() const
        {
            return _formats;
        }

        void EGL::GetModifiers(const uint32_t format, std::vector<uint64_t>& modifiers, std::vector<EGLBoolean>& externals) const
        {
            if (_api.eglQueryDmaBufModifiersEXT != nullptr) {
                EGLint nModifiers(0);
                _api.eglQueryDmaBufModifiersEXT(_display, format, 0, nullptr, nullptr, &nModifiers);
                modifiers.resize(nModifiers);
                externals.resize(nModifiers);

                _api.eglQueryDmaBufModifiersEXT(_display, format, nModifiers, modifiers.data(), externals.data(), &nModifiers);
            } else {
                modifiers = { DRM_FORMAT_MOD_LINEAR };
                externals = { EGL_FALSE };
            }
        }

        void EGL::GetPixelFormats(std::vector<PixelFormat>& pixelFormats) const
        {
            pixelFormats.clear();

            std::vector<int> formats;

            TRACE(Trace::EGL, ("Scanning for pixel formats"));

            if (_api.eglQueryDmaBufFormatsEXT != nullptr) {
                EGLint nFormats(0);
                _api.eglQueryDmaBufFormatsEXT(_display, 0, NULL, &nFormats);

                formats.resize(nFormats);
                _api.eglQueryDmaBufFormatsEXT(_display, formats.size(), formats.data(), &nFormats);
            } else {
                formats = { DRM_FORMAT_ARGB8888, DRM_FORMAT_XRGB8888 };
            }

            for (const auto& format : formats) {
                std::vector<uint64_t> modifiers;
                /**
                 * Indicates if the matching modifier is only supported for
                 * use with the GL_TEXTURE_EXTERNAL_OES texture target
                 */
                std::vector<EGLBoolean> externals;

                std::stringstream line;

                GetModifiers(format, modifiers, externals);

                char* formatName = drmGetFormatName(format);

                line << formatName << ", modifiers: ";

                free(formatName);

                for (uint8_t i(0); i < modifiers.size(); i++) {
                    char* modifierName = drmGetFormatModifierName(modifiers.at(i));

                    if (modifierName) {
                        line << modifierName << (externals.at(i) ? "*" : "");
                        free(modifierName);
                    }

                    line << ((i < (modifiers.size() - 1)) ? ", " : "");
                }

                TRACE(Trace::EGL, (_T("FormatInfo: %s"), line.str().c_str()));

                pixelFormats.emplace_back(format, modifiers);
            }
        }

        bool EGL::IsExternOnly(const uint32_t format, const uint64_t modifier) const
        {
            bool result(false);

            if (_api.eglQueryDmaBufModifiersEXT != nullptr) {
                std::vector<uint64_t> modifiers;
                std::vector<EGLBoolean> externals;

                GetModifiers(format, modifiers, externals);

                auto it = std::find(modifiers.begin(), modifiers.end(), modifier);

                if (it != modifiers.end()) {
                    result = externals[distance(modifiers.begin(), it)];
                }
            }

            return result;
        }

        EGLImage EGL::CreateImage(/*const*/ Exchange::ICompositionBuffer* buffer, bool& external) const
        {
            ASSERT(buffer != nullptr);
            Exchange::ICompositionBuffer::IIterator* planes = buffer->Acquire(Compositor::DefaultTimeoutMs);
            ASSERT(planes != nullptr);

            planes->Next();
            ASSERT(planes->IsValid() == true);

            EGLImage result(EGL_NO_IMAGE);

            if (_api.eglCreateImage != nullptr) {
                API::Attributes<EGLAttrib> imageAttributes;

                imageAttributes.Append(EGL_WIDTH, buffer->Width());
                imageAttributes.Append(EGL_HEIGHT, buffer->Height());
                imageAttributes.Append(EGL_LINUX_DRM_FOURCC_EXT, buffer->Format());

                TRACE(Trace::Information, ("plane 0 info fd=%" PRIuPTR " stride=%d offset=%d", planes->Descriptor(), planes->Stride(), planes->Offset()));

                imageAttributes.Append(EGL_DMA_BUF_PLANE0_FD_EXT, planes->Descriptor());
                imageAttributes.Append(EGL_DMA_BUF_PLANE0_OFFSET_EXT, planes->Offset());
                imageAttributes.Append(EGL_DMA_BUF_PLANE0_PITCH_EXT, planes->Stride());
                imageAttributes.Append(EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (buffer->Modifier() & 0xFFFFFFFF));
                imageAttributes.Append(EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (buffer->Modifier() >> 32));

                if (planes->Next() == true) {
                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_FD_EXT, planes->Descriptor());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_OFFSET_EXT, planes->Offset());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_PITCH_EXT, planes->Stride());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, (buffer->Modifier() & 0xFFFFFFFF));
                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, (buffer->Modifier() >> 32));
                }

                if (planes->Next() == true) {
                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_FD_EXT, planes->Descriptor());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_OFFSET_EXT, planes->Offset());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_PITCH_EXT, planes->Stride());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT, (buffer->Modifier() & 0xFFFFFFFF));
                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT, (buffer->Modifier() >> 32));
                }

                if (planes->Next() == true) {
                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_FD_EXT, planes->Descriptor());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_OFFSET_EXT, planes->Offset());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_PITCH_EXT, planes->Stride());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT, (buffer->Modifier() & 0xFFFFFFFF));
                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT, (buffer->Modifier() >> 32));
                }

                imageAttributes.Append(EGL_IMAGE_PRESERVED_KHR, EGL_TRUE);

                // #ifdef __DEBUG__
                //                 std::stringstream hexLine;

                //                 for (uint16_t i(0); i < imageAttributes.Size(); i++) {
                //                     const int att(imageAttributes[size_t(i)]);

                //                     hexLine << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << att << std::nouppercase;

                //                     if (i < imageAttributes.Size() - 1) {
                //                         hexLine << ", ";

                //                         if (((i + 1) % 2) == 0) {
                //                             hexLine << std::endl;
                //                         }
                //                     }
                //                 }

                //                 TRACE(Trace::EGL, ("%zu imageAttributes set: \n%s", imageAttributes.Size(), hexLine.str().c_str()));
                // #endif

                result = _api.eglCreateImage(_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, imageAttributes);

                TRACE(Trace::EGL, ("EGL image created result:%s", API::EGL::ErrorString(eglGetError())));
                ASSERT(eglGetError() == EGL_SUCCESS);
            }

            external = IsExternOnly(buffer->Format(), buffer->Modifier());

            // just unlock and go, client still need to draw something,.
            buffer->Relinquish();

            return result;
        }

        bool EGL::DestroyImage(EGLImage image) const
        {
            bool result((_api.eglDestroyImage != nullptr) && (image == EGL_NO_IMAGE));

            if ((_api.eglDestroyImage != nullptr) && (image != EGL_NO_IMAGE)) {
                result = (_api.eglDestroyImage(_display, image) == EGL_TRUE);
            }

            return result;
        }
    } // namespace Renderer
} // namespace Compositor
} // namespace Thunder
