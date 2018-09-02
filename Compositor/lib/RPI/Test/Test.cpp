#include <string>
#include <chrono>
#include <iostream>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "../../Client/Client.h"

namespace WPEFramework {

EGLint const attrib_list[] = {
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8,
        EGL_STENCIL_SIZE,
        0,
        EGL_BUFFER_SIZE,
        32,
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_COLOR_BUFFER_TYPE,
        EGL_RGB_BUFFER,
        EGL_CONFORMANT,
        EGL_OPENGL_ES2_BIT,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_NONE
};

EGLint context_attrib_list[] = {
        EGL_CONTEXT_CLIENT_VERSION,
        2,
        EGL_NONE,
};

class TestContext {

public:

    int run() {
        printf("q = quit, n = new Surface, d = delete surface\n");
        idisplay = Compositor::IDisplay::Instance(DisplayName   ());
        setupEGL();

        int character;
        do {
            printf("\n>>");
            character = ::toupper(::getc(stdin));

            switch (character) {
            case 'N': {
                std::string name;
                printf("\nEnter name >>");
                std::cin.ignore();
                std::getline(std::cin, name);
                createSurface(name);
                drawFrame();
                break;
            }
            case 'D': {
                destroySurface();
                break;
            }
            }
        } while (character != 'Q');

        idisplay->Release();

        termEGL();
        return (0);
    }

private:

    void drawFrame() {

        GLclampf red = ((float)rand()/(float)(RAND_MAX)) * 1.0;
        GLclampf green = ((float)rand()/(float)(RAND_MAX)) * 1.0;
        GLclampf blue = ((float)rand()/(float)(RAND_MAX)) * 1.0;

        glClearColor(red, green, blue, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();

        eglSwapBuffers(eglDisplay, eglSurfaceWindow);
    }

    void createSurface(const std::string& name) {

        if ( surface != nullptr )
        {
            surface->Release();
            surface = nullptr;        
        }

        surface = idisplay->Create(name, 1280, 720);
        NativeWindowType native = surface->Native();
        eglSurfaceWindow = eglCreateWindowSurface(
                eglDisplay, eglConfig, native, NULL);
        eglMakeCurrent(
                eglDisplay, eglSurfaceWindow, eglSurfaceWindow, eglContext);
    }

    void destroySurface() {

        eglMakeCurrent(
                eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(eglDisplay, eglSurfaceWindow);

        if ( surface != nullptr )
        {
            surface->Release();
            surface = nullptr;        
        }
    }

    void setupEGL() {

        eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(eglDisplay, NULL, NULL);

        EGLint configCount = 0;
        eglChooseConfig(eglDisplay, attrib_list, &eglConfig, 1, &configCount);
        eglContext = eglCreateContext(
                eglDisplay, eglConfig, EGL_NO_CONTEXT, context_attrib_list);
    }

    void termEGL() {

        eglDestroyContext(eglDisplay, eglContext);
        eglTerminate(eglDisplay);
    }

    std::string DisplayName() {
        std::string name;
        const char* callsign (std::getenv("CLIENT_IDENTIFIER"));

        if (callsign == nullptr) {
            name = "WebKitBrowser" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        }
        else {
            const char* delimiter = nullptr;
            if ((delimiter = strchr(callsign, ',')) == nullptr) {
                name = callsign;
            }
            else {
                name = std::string(callsign, (delimiter - callsign));
            }
        }
        return (name);
    }

private:

    Compositor::IDisplay *idisplay = nullptr;
    Compositor::IDisplay::ISurface* surface = nullptr;

    EGLDisplay eglDisplay;
    EGLConfig eglConfig;
    EGLContext eglContext;
    EGLSurface eglSurfaceWindow;
};
} // WPEFramework

int main(int argc, char ** argv)
{
    srand(time(0));
    WPEFramework::TestContext *tcontext = new WPEFramework::TestContext();
    tcontext->run();
    return (0);
}
