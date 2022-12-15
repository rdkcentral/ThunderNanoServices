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

// #include <Module.h>

#ifndef EGL_NO_X11
#define EGL_NO_X11
#endif
#ifndef EGL_NO_PLATFORM_SPECIFIC_TYPES
#define EGL_NO_PLATFORM_SPECIFIC_TYPES
#endif

// #include <core/core.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <drm_fourcc.h>
#include <gbm.h>

namespace Compositor {
namespace Renderer {
    class EGL {
    public:
        EGL(EGL const&) = delete;
        EGL& operator=(EGL const&) = delete;

        EGL()
            : _display(EGL_NO_DISPLAY)
            , _context(EGL_NO_CONTEXT)
        {
        }

    public:
        bool IsCurrent() const
        {
            return eglGetCurrentContext() == _context;
        }

        bool SetCurrent()
        {
            return (eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, _context) == EGL_TRUE);
        }

        bool ResetCurrent()
        {
            return (eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE);
        }

    private:
        EGLDisplay _display;
        EGLContext _context;
    }; // class EGL
} // namespace Renderer
} // namespace Compositor