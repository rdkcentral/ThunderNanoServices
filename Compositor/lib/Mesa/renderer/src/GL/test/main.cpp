#ifndef EGL_NO_X11
#define EGL_NO_X11
#endif
#ifndef EGL_NO_PLATFORM_SPECIFIC_TYPES
#define EGL_NO_PLATFORM_SPECIFIC_TYPES
#endif

#ifndef MODULE_NAME
#define MODULE_NAME CompositorGLTest
#endif

#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "../Trace.h"
#include <localtracer/localtracer.h>
#include <tracing/tracing.h>

#include "../EGL.h"
#include "../RenderAPI.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

const std::vector<std::string> Parse(const std::string& input)
{
    std::istringstream iss(input);
    return std::vector<std::string> { std::istream_iterator<std::string> { iss }, std::istream_iterator<std::string> {} };
}

int main(int argc, const char* argv[])
{
    // EGLDisplay dpy = EGL_NO_DISPLAY;

    // const std::string extensions(eglQueryString(dpy, EGL_EXTENSIONS));
    // std::cout << "extensions:" << std::endl;

    // for (auto& s : Parse(extensions)) {
    //     std::cout << " - " << s << std::endl;
    // }

    WPEFramework::Messaging::LocalTracer& tracer = WPEFramework::Messaging::LocalTracer::Open();
    WPEFramework::Messaging::ConsolePrinter printer(false);

    tracer.Callback(&printer);

    std::vector<string> modules = {
        "Error",
        "Information"
        "EGL"
    };

    for (auto module : modules) {
        tracer.EnableMessage("CompositorGLTest", module, true);
        tracer.EnableMessage("CompositorRendererGL", module, true);
    }

    // Compositor::API::GL gl_api;
    // Compositor::API::EGL egl_api;

    int drmFd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);

    Compositor::Renderer::EGL egl(drmFd);

    close(drmFd);

    // dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    // EGLint major = 0, minor = 0;
    // eglInitialize(dpy, &major, &minor) != EGL_FALSE;

    // const std::string apis(eglQueryString(dpy, EGL_CLIENT_APIS));
    // std::cout << "apis" << std::endl;
    // for (auto& s : Parse(apis)) {
    //     std::cout << " - " << s << std::endl;
    // }

    return 0;
}