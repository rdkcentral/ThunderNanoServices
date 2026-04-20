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

#ifndef EGL_NO_X11
#define EGL_NO_X11
#endif
#ifndef EGL_NO_PLATFORM_SPECIFIC_TYPES
#define EGL_NO_PLATFORM_SPECIFIC_TYPES
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <CompositorTypes.h>
#include <interfaces/IGraphicsBuffer.h>

#include "RenderAPI.h"

struct gbm_device;
namespace Thunder {
namespace Compositor {
    namespace Renderer {
        class EGL {
        public:
            EGL() = delete;
            EGL(EGL&&) = delete;
            EGL(const EGL&) = delete;
            EGL& operator=(EGL&&) = delete;
            EGL& operator=(const EGL&) = delete;

            EGL(const int drmFd);
            ~EGL();

        public:
            inline EGLDisplay Display() const
            {
                return _display;
            }

            inline EGLContext Context() const
            {
                return _context;
            }

            inline bool IsCurrent() const
            {
                return eglGetCurrentContext() == _context;
            }

            inline bool SetCurrent() const
            {
                return (_display != EGL_NO_DISPLAY) ? (eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, _context) == EGL_TRUE) : false;
            }

            inline bool ResetCurrent() const
            {
                return (_display != EGL_NO_DISPLAY) ? (eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE) : false;
            }

            const std::vector<PixelFormat>& TextureFormats() const
            {
                return _textureFormats;
            }
            const std::vector<PixelFormat>& RenderFormats() const
            {
                return _renderFormats;
            }

            EGLImage CreateImage(/*const*/ Exchange::IGraphicsBuffer* buffer, bool&) const;

            bool DestroyImage(EGLImage image) const;

            class ContextBackup {
            public:
                ContextBackup(const ContextBackup&) = delete;
                ContextBackup& operator=(const ContextBackup&) = delete;
                ContextBackup& operator=(ContextBackup&&) = delete;

                ContextBackup()
                    : _display(eglGetCurrentDisplay())
                    , _context(eglGetCurrentContext())
                    , _drawSurface(eglGetCurrentSurface(EGL_DRAW))
                    , _readSurface(eglGetCurrentSurface(EGL_READ))
                {
                }

                ~ContextBackup()
                {
                    if ((_display != EGL_NO_DISPLAY && _context != EGL_NO_CONTEXT)) {
                        eglMakeCurrent(_display, _drawSurface, _readSurface, _context);
                    } else {
                        EGLDisplay currentDisplay = eglGetCurrentDisplay();

                        if (currentDisplay != EGL_NO_DISPLAY) {
                            eglMakeCurrent(currentDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                        }
                    }
                }

            private:
                EGLDisplay _display;
                EGLContext _context;
                EGLSurface _drawSurface;
                EGLSurface _readSurface;
            };

            void DestroySync(EGLSync& sync) const
            {
                if (sync != EGL_NO_SYNC && _api.eglDestroySync != nullptr) {
                    _api.eglDestroySync(_display, sync);
                    sync = EGL_NO_SYNC;
                }
            }

            EGLSync CreateSync() const
            {
                EGLSync sync = EGL_NO_SYNC;

                if (_api.eglCreateSync != nullptr) {
                    sync = _api.eglCreateSync(_display, EGL_SYNC_FENCE, nullptr);
                }

                return sync;
            }

            EGLint WaitSync(EGLSync& sync, const EGLTime timeoutNs = EGL_FOREVER) const
            {
                EGLint status = EGL_UNSIGNALED;

                if (sync != EGL_NO_SYNC && _api.eglWaitSync != nullptr) {
                    status = _api.eglClientWaitSync(_display, sync, EGL_SYNC_FLUSH_COMMANDS_BIT, timeoutNs);
                }

                return status;
            }

            int ExportSyncAsFd(EGLSync sync) const
            {
                int fd = -1;

                if (sync != EGL_NO_SYNC && _api.eglDupNativeFenceFDANDROID != nullptr) {
                    fd = _api.eglDupNativeFenceFDANDROID(_display, sync);

                    if (fd < 0) {
                        TRACE(Trace::Error, ("Failed to export sync as fd: 0x%x", eglGetError()));
                    }
                }

                return fd;
            }

        private:
            EGLDeviceEXT FindEGLDevice(const int drmFd);
            uint32_t Initialize(EGLenum platform, void* remote_display, bool isMaster);
            void GetPixelFormats(std::vector<PixelFormat>& formats, const bool renderOnly) const;
            void GetModifiers(const uint32_t format, std::vector<uint64_t>& modifiers, std::vector<EGLBoolean>& externals) const;
            bool IsExternOnly(const uint32_t format, const uint64_t modifier) const;

        private:
            const API::EGL& _api;

            EGLDisplay _display;
            EGLContext _context;

            EGLDeviceEXT _device;

            std::vector<PixelFormat> _renderFormats;
            std::vector<PixelFormat> _textureFormats;

            int _gbmDescriptor;
            gbm_device* _gbmDevice;

            int _drmFd;
        }; // class EGL
    } // namespace Renderer
} // namespace Compositor
} // namespace Thunder
