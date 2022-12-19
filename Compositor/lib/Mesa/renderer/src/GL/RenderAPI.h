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

#ifndef MODULE_NAME
#define MODULE_NAME CompositorRenderEGL
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

namespace Compositor {

#ifndef EGL_VERSION_1_5
using PFNEGLCREATEIMAGEPROC = PFNEGLCREATEIMAGEKHRPROC;
using PFNEGLDESTROYIMAGEPROC = PFNEGLDESTROYIMAGEKHRPROC;
using PFNEGLCREATESYNCPROC = PFNEGLCREATESYNCKHRPROC;
using PFNEGLDESTROYSYNCPROC = PFNEGLDESTROYSYNCKHRPROC;
using PFNEGLWAITSYNCPROC = PFNEGLWAITSYNCKHRPROC;
using PFNEGLCLIENTWAITSYNCPROC = PFNEGLCLIENTWAITSYNCKHRPROC;

constexpr char eglCreateImageProc[] = "eglCreateImageKHR";
constexpr char eglDestroyImageProc[] = "eglDestroyImageKHR";
constexpr char eglCreateSyncProc[] = "eglCreateSyncKHR";
constexpr char eglDestroySyncProc[] = "eglDestroySyncKHR";
constexpr char eglWaitSyncProc[] = "eglWaitSyncKHR";
constexpr char eglClientWaitSyncProc[] = "eglClientWaitSyncKHR";
#else
constexpr char eglCreateImageProc[] = "eglCreateImage";
constexpr char eglDestroyImageProc[] = "eglDestroyImage";
constexpr char eglCreateSyncProc[] = "eglCreateSync";
constexpr char eglDestroySyncProc[] = "eglDestroySync";
constexpr char eglWaitSyncProc[] = "eglWaitSync";
constexpr char eglClientWaitSyncProc[] = "eglClientWaitSync";
#endif

namespace API {
    template <class TYPE>
    class Attributes {
    protected:
        std::vector<TYPE> attributes;
        typename std::vector<TYPE>::iterator pos;

    public:
        Attributes(TYPE terminator = EGL_NONE)
        {
            attributes.push_back(terminator);
        }

        void append(TYPE param)
        {
            attributes.insert(attributes.end() - 1, param);
        }

        void append(TYPE name, TYPE value)
        {
            append(name);
            append(value);
        }

        operator TYPE*(void)
        {
            return &attributes[0];
        }

        operator const TYPE*(void) const
        {
            return &attributes[0];
        }
    };

    class GL {
    public:
        static inline std::string ShaderInfoLog(GLuint handle)
        {

            std::string log;

            if (glIsShader(handle) == true) {
                GLint length;

                glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);

                if (length > 1) {
                    log.resize(length);
                    glGetShaderInfoLog(handle, length, NULL, &log[0]);
                }
            } else {
                log = ("Handle is a invalid shader handle");
            }

            return log;
        }

        static inline std::string ProgramInfoLog(GLuint handle)
        {
            std::string log;
            if (glIsProgram(handle)) {
                GLint length(0);

                glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);

                if (length > 1) {
                    log.resize(length);
                    glGetProgramInfoLog(handle, length, NULL, &log[0]);
                }
            } else {
                log = ("Handle is a invalid program handle");
            }

            return log;
        }

        static inline bool HasRenderer(const std::string& name)
        {
            static const std::string renderer(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
            return ((name.size() > 0) && (renderer.find(name) != std::string::npos));
        }

        static inline bool HasExtension(const std::string& extention)
        {
            static const std::string extensions(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
            return ((extention.size() > 0) && (extensions.find(extention) != std::string::npos));
        }

        GL(const GL&) = delete;
        GL& operator=(const GL&) = delete;

        GL()
            : glEGLImageTargetTexture2DOES(nullptr)
            , glEGLImageTargetRenderbufferStorageOES(nullptr)
            , glDebugMessageCallbackKHR(nullptr)
            , glDebugMessageControlKHR(nullptr)
            , glPopDebugGroupKHR(nullptr)
            , glPushDebugGroupKHR(nullptr)
            , glGetGraphicsResetStatusKHR(nullptr)
        {
            glEGLImageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
            glEGLImageTargetRenderbufferStorageOES = reinterpret_cast<PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC>(eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES"));
            glDebugMessageCallbackKHR = reinterpret_cast<PFNGLDEBUGMESSAGECALLBACKKHRPROC>(eglGetProcAddress("glDebugMessageCallbackKHR"));
            glDebugMessageControlKHR = reinterpret_cast<PFNGLDEBUGMESSAGECONTROLKHRPROC>(eglGetProcAddress("glDebugMessageControlKHR"));
            glPopDebugGroupKHR = reinterpret_cast<PFNGLPOPDEBUGGROUPKHRPROC>(eglGetProcAddress("glPopDebugGroupKHR"));
            glPushDebugGroupKHR = reinterpret_cast<PFNGLPUSHDEBUGGROUPKHRPROC>(eglGetProcAddress("glPushDebugGroupKHR"));
            glGetGraphicsResetStatusKHR = reinterpret_cast<PFNGLGETGRAPHICSRESETSTATUSKHRPROC>(eglGetProcAddress("glGetGraphicsResetStatusKHR"));
        }

        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
        PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glEGLImageTargetRenderbufferStorageOES;
        PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallbackKHR;
        PFNGLDEBUGMESSAGECONTROLKHRPROC glDebugMessageControlKHR;
        PFNGLPOPDEBUGGROUPKHRPROC glPopDebugGroupKHR;
        PFNGLPUSHDEBUGGROUPKHRPROC glPushDebugGroupKHR;
        PFNGLGETGRAPHICSRESETSTATUSKHRPROC glGetGraphicsResetStatusKHR;
    }; // class GL

    class EGL {
    public:
#define CASE_STR(value) \
    case value:         \
        return #value;

        static const char* ErrorString(EGLint code)
        {
            switch (code) {
                CASE_STR(EGL_SUCCESS)
                CASE_STR(EGL_NOT_INITIALIZED)
                CASE_STR(EGL_BAD_ACCESS)
                CASE_STR(EGL_BAD_ALLOC)
                CASE_STR(EGL_BAD_ATTRIBUTE)
                CASE_STR(EGL_BAD_CONTEXT)
                CASE_STR(EGL_BAD_CONFIG)
                CASE_STR(EGL_BAD_CURRENT_SURFACE)
                CASE_STR(EGL_BAD_DISPLAY)
                CASE_STR(EGL_BAD_SURFACE)
                CASE_STR(EGL_BAD_MATCH)
                CASE_STR(EGL_BAD_PARAMETER)
                CASE_STR(EGL_BAD_NATIVE_PIXMAP)
                CASE_STR(EGL_BAD_NATIVE_WINDOW)
                CASE_STR(EGL_CONTEXT_LOST)
            default:
                return "Unknown";
            }
        }
#undef CASE_STR

        static inline bool HasExtension(const std::string& extention, EGLDisplay dpy = eglGetCurrentDisplay())
        {
            static const std::string extensions(eglQueryString(dpy, EGL_EXTENSIONS));
            return ((extention.size() > 0) && (extensions.find(extention) != std::string::npos));
        }

        static inline bool HasClientAPI(const std::string& api, EGLDisplay dpy = eglGetCurrentDisplay())
        {
            // ASSERT(dpy != EGL_NO_DISPLAY);
            static const std::string apis(eglQueryString(dpy, EGL_CLIENT_APIS));
            return ((api.size() > 0) && (apis.find(api) != std::string::npos));
        }

        EGL(const EGL&) = delete;
        EGL& operator=(const EGL&) = delete;

        EGL()
            : eglGetPlatformDisplayEXT(nullptr)
            , eglQueryDmaBufFormatsEXT(nullptr)
            , eglQueryDmaBufModifiersEXT(nullptr)
            , eglQueryDisplayAttribEXT(nullptr)
            , eglQueryDeviceStringEXT(nullptr)
            , eglQueryDevicesEXT(nullptr)
            , eglDebugMessageControl(nullptr)
            , eglQueryDebug(nullptr)
            , eglLabelObject(nullptr)
            , eglCreateImage(nullptr)
            , eglDestroyImage(nullptr)
            , eglCreateSync(nullptr)
            , eglDestroySync(nullptr)
            , eglWaitSync(nullptr)
            , eglClientWaitSync(nullptr)
        {
            eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
            eglQueryDmaBufFormatsEXT = reinterpret_cast<PFNEGLQUERYDMABUFFORMATSEXTPROC>(eglGetProcAddress("eglQueryDmaBufFormatsEXT"));
            eglQueryDmaBufModifiersEXT = reinterpret_cast<PFNEGLQUERYDMABUFMODIFIERSEXTPROC>(eglGetProcAddress("eglQueryDmaBufModifiersEXT"));
            eglQueryDisplayAttribEXT = reinterpret_cast<PFNEGLQUERYDISPLAYATTRIBEXTPROC>(eglGetProcAddress("eglQueryDisplayAttribEXT"));
            eglQueryDeviceStringEXT = reinterpret_cast<PFNEGLQUERYDEVICESTRINGEXTPROC>(eglGetProcAddress("eglQueryDeviceStringEXT"));
            eglQueryDevicesEXT = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(eglGetProcAddress("eglQueryDevicesEXT"));

            eglDebugMessageControl = reinterpret_cast<PFNEGLDEBUGMESSAGECONTROLKHRPROC>(eglGetProcAddress("eglDebugMessageControlKHR"));
            eglQueryDebug = reinterpret_cast<PFNEGLQUERYDEBUGKHRPROC>(eglGetProcAddress("eglQueryDebugKHR"));
            eglLabelObject = reinterpret_cast<PFNEGLLABELOBJECTKHRPROC>(eglGetProcAddress("eglLabelObjectKHR"));
            eglCreateImage = reinterpret_cast<PFNEGLCREATEIMAGEPROC>(eglGetProcAddress(eglCreateImageProc));
            eglDestroyImage = reinterpret_cast<PFNEGLDESTROYIMAGEPROC>(eglGetProcAddress(eglDestroyImageProc));
            eglCreateSync = reinterpret_cast<PFNEGLCREATESYNCPROC>(eglGetProcAddress(eglCreateSyncProc));
            eglDestroySync = reinterpret_cast<PFNEGLDESTROYSYNCPROC>(eglGetProcAddress(eglDestroySyncProc));
            eglWaitSync = reinterpret_cast<PFNEGLWAITSYNCPROC>(eglGetProcAddress(eglWaitSyncProc));
            eglClientWaitSync = reinterpret_cast<PFNEGLCLIENTWAITSYNCPROC>(eglGetProcAddress(eglClientWaitSyncProc));
        }

    public:
        PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;

        PFNEGLQUERYDMABUFFORMATSEXTPROC eglQueryDmaBufFormatsEXT;
        PFNEGLQUERYDMABUFMODIFIERSEXTPROC eglQueryDmaBufModifiersEXT;
        PFNEGLQUERYDISPLAYATTRIBEXTPROC eglQueryDisplayAttribEXT;
        PFNEGLQUERYDEVICESTRINGEXTPROC eglQueryDeviceStringEXT;
        PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT;

        PFNEGLDEBUGMESSAGECONTROLKHRPROC eglDebugMessageControl;
        PFNEGLQUERYDEBUGKHRPROC eglQueryDebug;
        PFNEGLLABELOBJECTKHRPROC eglLabelObject;
        PFNEGLCREATEIMAGEPROC eglCreateImage;
        PFNEGLDESTROYIMAGEPROC eglDestroyImage;
        PFNEGLCREATESYNCPROC eglCreateSync;
        PFNEGLDESTROYSYNCPROC eglDestroySync;
        PFNEGLWAITSYNCPROC eglWaitSync;
        PFNEGLCLIENTWAITSYNCPROC eglClientWaitSync;
    }; // class EGL

} // namespace API
} // namespace Compositor
