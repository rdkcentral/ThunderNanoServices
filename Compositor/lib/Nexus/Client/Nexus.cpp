#include "Implementation.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <poll.h>

namespace WPEFramework {

namespace Nexus {

#define Trace(fmt, args...) fprintf(stderr, "[pid=%d][Client %s:%d] : " fmt, getpid(), __FILE__, __LINE__, ##args)
#define RED_SIZE (8)
#define GREEN_SIZE (8)
#define BLUE_SIZE (8)
#define ALPHA_SIZE (8)
#define DEPTH_SIZE (0)

    static void printEGLConfiguration(EGLDisplay dpy, EGLConfig config)
    {
#define X(VAL)    \
    {             \
        VAL, #VAL \
    }
        struct {
            EGLint attribute;
            const char* name;
        } names[] = {
            X(EGL_BUFFER_SIZE),
            X(EGL_RED_SIZE),
            X(EGL_GREEN_SIZE),
            X(EGL_BLUE_SIZE),
            X(EGL_ALPHA_SIZE),
            X(EGL_CONFIG_CAVEAT),
            X(EGL_CONFIG_ID),
            X(EGL_DEPTH_SIZE),
            X(EGL_LEVEL),
            X(EGL_MAX_PBUFFER_WIDTH),
            X(EGL_MAX_PBUFFER_HEIGHT),
            X(EGL_MAX_PBUFFER_PIXELS),
            X(EGL_NATIVE_RENDERABLE),
            X(EGL_NATIVE_VISUAL_ID),
            X(EGL_NATIVE_VISUAL_TYPE),
            // X(EGL_PRESERVED_RESOURCES),
            X(EGL_SAMPLE_BUFFERS),
            X(EGL_SAMPLES),
            // X(EGL_STENCIL_BITS),
            X(EGL_SURFACE_TYPE),
            X(EGL_TRANSPARENT_TYPE),
            // X(EGL_TRANSPARENT_RED),
            // X(EGL_TRANSPARENT_GREEN),
            // X(EGL_TRANSPARENT_BLUE)
        };
#undef X

        for (int j = 0; j < sizeof(names) / sizeof(names[0]); j++) {
            EGLint value = -1;
            EGLBoolean res = eglGetConfigAttrib(dpy, config, names[j].attribute, &value);
            if (res) {
                Trace("  - %s: %d (0x%x)\n", names[j].name, value, value);
            }
        }
    }

    Display::SurfaceImplementation::SurfaceImplementation(Display& display, const std::string& name, const uint32_t width, const uint32_t height)
        : _refcount(1)
        , _name(name)
        , _x(0)
        , _y(0)
        , _width(width)
        , _height(height)
        , _eglSurfaceWindow(EGL_NO_SURFACE)
        , _keyboard(nullptr)
    {
    }

    void Display::Initialize()
    {
        /*
         * Get default EGL display
         */
        _eglDisplay = eglGetDisplay(reinterpret_cast<NativeDisplayType>(_eglDisplay));

        if (_eglDisplay == EGL_NO_DISPLAY) {
            Trace("Oops bad Display: %p\n", _eglDisplay);
        }
        else {
            /*
             * Initialize display
             */
            EGLint major, minor;
            if (eglInitialize(_eglDisplay, &major, &minor) != EGL_TRUE) {
                Trace("Unable to initialize EGL: %X\n", eglGetError());
            }
            else {
                /*
                 * Get number of available configurations
                 */
                EGLint configCount;
                Trace("Vendor: %s\n", eglQueryString(_eglDisplay, EGL_VENDOR));
                Trace("Version: %d.%d\n", major, minor);

                if (eglGetConfigs(_eglDisplay, nullptr, 0, &configCount)) {

                    EGLConfig eglConfigs[configCount];

                    EGLint attributes[] = {
                        EGL_RED_SIZE, RED_SIZE,
                        EGL_GREEN_SIZE, GREEN_SIZE,
                        EGL_BLUE_SIZE, BLUE_SIZE,
                        EGL_DEPTH_SIZE, DEPTH_SIZE,
                        EGL_STENCIL_SIZE, 0,
                        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                        EGL_NONE
                    };

                    Trace("Configs: %d\n", configCount);
                    /*
                     * Get a list of configurations that meet or exceed our requirements
                     */
                    if (eglChooseConfig(_eglDisplay, attributes, eglConfigs, configCount, &configCount)) {

                        /*
                         * Choose a suitable configuration
                         */
                        unsigned int index = 0;

                        while (index < configCount) {
                            EGLint redSize, greenSize, blueSize, alphaSize, depthSize;

                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_RED_SIZE, &redSize);
                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_GREEN_SIZE, &greenSize);
                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_BLUE_SIZE, &blueSize);
                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_ALPHA_SIZE, &alphaSize);
                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_DEPTH_SIZE, &depthSize);

                            if ((redSize == RED_SIZE) && (greenSize == GREEN_SIZE) && (blueSize == BLUE_SIZE) && (alphaSize == ALPHA_SIZE) && (depthSize >= DEPTH_SIZE)) {
                                break;
                            }

                            index++;
                        }
                        if (index < configCount) {
                            _eglConfig = eglConfigs[index];

                            EGLint attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2 /* ES2 */, EGL_NONE };

                            Trace("Config choosen: %d\n", index);
                            printEGLConfiguration(_eglDisplay, _eglConfig);

                            /*
                             * Create an EGL context
                             */
                            _eglContext = eglCreateContext(_eglDisplay, _eglConfig, EGL_NO_CONTEXT, attributes);

                            Trace("Context created\n");
                        }
                    }
                }
                Trace("Extentions: %s\n", eglQueryString(_eglDisplay, EGL_EXTENSIONS));
            }
        }
    }

    void Display::Deinitialize()
    {
        if (_eglContext != EGL_NO_CONTEXT) {
            eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglTerminate(_eglDisplay);
            eglReleaseThread();
        }
    }

    /* virtual */ Compositor::IDisplay::ISurface* Display::Create(const std::string& name, const uint32_t width, const uint32_t height)
    {
        IDisplay::ISurface* result = nullptr;

        return (result);
    }

    /* static */ Compositor::IDisplay* Display::Instance(const std::string& displayName)
    {
        Compositor::IDisplay* result(nullptr);

        return (result);
    }


    /* virtual */ int Display::FileDescriptor() const
    {
        return (_fd);
    }
}
    /* static */ Compositor::IDisplay* Compositor::IDisplay::Instance(const std::string& displayName) {
        return (Nexus::Display::Instance(displayName));
    }
}
