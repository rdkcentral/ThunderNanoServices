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
#include <interfaces/ICompositionBuffer.h>

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
                return (eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, _context) == EGL_TRUE);
            }

            inline bool ResetCurrent() const
            {
                return (eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE);
            }

            const std::vector<PixelFormat>& Formats() const;

            EGLImage CreateImage(/*const*/ Exchange::ICompositionBuffer* buffer, bool&) const;

            bool DestroyImage(EGLImage image) const;

            class ContextBackup {
            public:
                ContextBackup(const ContextBackup&) = delete;
                ContextBackup& operator=(const ContextBackup&) = delete;

                ContextBackup()
                    : _display(eglGetCurrentDisplay())
                    , _context(eglGetCurrentContext())
                    , _drawSurface(eglGetCurrentSurface(EGL_DRAW))
                    , _readSurface(eglGetCurrentSurface(EGL_READ))
                {
                }

                ~ContextBackup()
                {
                    eglMakeCurrent((_display == EGL_NO_DISPLAY) ? eglGetCurrentDisplay() : _display, _drawSurface, _readSurface, _context);
                }

            private:
                EGLDisplay _display;
                EGLContext _context;
                EGLSurface _drawSurface;
                EGLSurface _readSurface;
            };

        private:
            EGLDeviceEXT FindEGLDevice(const int drmFd);
            uint32_t Initialize(EGLenum platform, void* remote_display, bool isMaster);
            void GetPixelFormats(std::vector<PixelFormat>& formats) const;
            void GetModifiers(const uint32_t format, std::vector<uint64_t>& modifiers, std::vector<EGLBoolean>& externals) const;
            bool IsExternOnly(const uint32_t format, const uint64_t modifier) const;

        private:
            API::EGL _api;

            EGLDisplay _display;
            EGLContext _context;

            EGLDeviceEXT _device;

            EGLSurface _draw_surface;
            EGLSurface _read_surface;

            std::vector<PixelFormat> _formats;

            int _gbmDescriptor;
            gbm_device* _gbmDevice;
        }; // class EGL
    } // namespace Renderer
} // namespace Compositor
} // namespace Thunder
