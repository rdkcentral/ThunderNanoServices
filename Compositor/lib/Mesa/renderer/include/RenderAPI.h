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

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

namespace Render {
class API {
public:
    API(const API&) = delete;
    API& operator=(const API&) = delete;

    API()
        : eglGetPlatformDisplayEXT(nullptr)
        , glEGLImageTargetTexture2DOES(nullptr)
        , eglCreateImage(nullptr)
        , eglDestroyImage(nullptr)
        , eglCreateSync(nullptr)
        , eglDestroySync(nullptr)
        , eglWaitSync(nullptr)
        , eglClientWaitSync(nullptr)
    {
        eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
        glEGLImageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

#ifdef EGL_VERSION_1_5
        eglCreateImage = reinterpret_cast<PFNEGLCREATEIMAGEPROC>(eglGetProcAddress("eglCreateImage"));
        eglDestroyImage = reinterpret_cast<PFNEGLDESTROYIMAGEPROC>(eglGetProcAddress("eglDestroyImage"));
        eglCreateSync = reinterpret_cast<PFNEGLCREATESYNCPROC>(eglGetProcAddress("eglCreateSync"));
        eglDestroySync = reinterpret_cast<PFNEGLDESTROYSYNCPROC>(eglGetProcAddress("eglDestroySync"));
        eglWaitSync = reinterpret_cast<PFNEGLWAITSYNCPROC>(eglGetProcAddress("eglWaitSync"));
        eglClientWaitSync = reinterpret_cast<PFNEGLCLIENTWAITSYNCPROC>(eglGetProcAddress("eglClientWaitSync"));
#else
        eglCreateImage = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        eglDestroyImage = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
        eglCreateSync = reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(eglGetProcAddress("eglCreateSyncKHR"));
        eglDestroySync = reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(eglGetProcAddress("eglDestroySyncKHR"));
        eglWaitSync = reinterpret_cast<PFNEGLWAITSYNCKHRPROC>(eglGetProcAddress("eglWaitSyncKHR"));
        eglClientWaitSync = reinterpret_cast<PFNEGLCLIENTWAITSYNCKHRPROC>(eglGetProcAddress("eglClientWaitSyncKHR"));
#endif
    }

public:
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

#ifdef EGL_VERSION_1_5
    PFNEGLCREATEIMAGEPROC eglCreateImage;
    PFNEGLDESTROYIMAGEPROC eglDestroyImage;
    PFNEGLCREATESYNCPROC eglCreateSync;
    PFNEGLDESTROYSYNCPROC eglDestroySync;
    PFNEGLWAITSYNCPROC eglWaitSync;
    PFNEGLCLIENTWAITSYNCPROC eglClientWaitSync;
#else
    PFNEGLCREATEIMAGEKHRPROC eglCreateImage;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImage;
    PFNEGLCREATESYNCKHRPROC eglCreateSync;
    PFNEGLDESTROYSYNCKHRPROC eglDestroySync;
    PFNEGLWAITSYNCKHRPROC eglWaitSync;
    PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSync;
#endif
}; // class API

namespace EGL {
    static bool HasExtension(const EGLDisplay dpy, const std::string& extention)
    {
        const std::string extensions(eglQueryString(dpy, EGL_EXTENSIONS));
        return ((extention.size() > 0) && (extensions.find(extention) != std::string::npos));
    }

    static bool HasAPI(const EGLDisplay dpy, const std::string& api)
    {
        const std::string apis(eglQueryString(dpy, EGL_CLIENT_APIS));
        return ((api.size() > 0) && (apis.find(api) != std::string::npos));
    }
} // namespace EGL

namespace GL {
    static bool HasRenderer(const std::string& name)
    {
        static const std::string renderer(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        return ((name.size() > 0) && (renderer.find(name) != std::string::npos));
    }

    static bool HasExtension(const std::string& extention)
    {
        static const std::string extensions(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
        return ((extention.size() > 0) && (extensions.find(extention) != std::string::npos));
    }
} // namespace GL
} // namespace Render
