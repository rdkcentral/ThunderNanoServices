#include <string>
#include <chrono>
#include <iostream>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "../../Client/Client.h"

namespace WPEFramework {

class Keyboard : public Compositor::IDisplay::IKeyboard {
private:
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

public:
    Keyboard() {
    }
    virtual ~Keyboard() {
    }

public:
    virtual uint32_t AddRef() const override {
    }
    virtual uint32_t Release() const override {
    }
    virtual void KeyMap(const char information[], const uint16_t size) override {
    }
    virtual void Key(const uint32_t key,
            const IKeyboard::state action, const uint32_t time) override {
    }
    virtual void Modifiers(uint32_t depressedMods,
            uint32_t latchedMods, uint32_t lockedMods, uint32_t group) override {
    }
    virtual void Repeat(int32_t rate, int32_t delay) override {
    }
    virtual void Direct(const uint32_t key, const state action) override {
        fprintf(stderr, "KEY: %u for %s\n", key, _isurface->Name().c_str());
    }
    void SetSurface(Compositor::IDisplay::ISurface* surface){
        _isurface = surface;
    }

private:
    Compositor::IDisplay::ISurface* _isurface;
};

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

#define MAX_SURFACES 5

class TestContext {

public:
    int run() {
        setupEGL();
        idisplay = Compositor::IDisplay::Instance(DisplayName());

        for (int i = 0; i < MAX_SURFACES; i++) {

            std::string name = "surface-" + std::to_string(i);
            isurfaces[i] = idisplay->Create(name, 1280/2, 720/2);
            eglSurfaceWindows[i] = createEGLSurface(isurfaces[i]->Native());

            ikeyboard[i] = new Keyboard();
            isurfaces[i]->Keyboard(ikeyboard[i]);
            ikeyboard[i]->SetSurface(isurfaces[i]);
        }

        for (int i = 0; i < MAX_SURFACES; i++) {

            drawFrame(eglSurfaceWindows[i]);
        }

        while (true) {

            idisplay->Process(1);
            usleep(100000);
        }

        for (int i = 0; i < MAX_SURFACES; i++) {

            delete ikeyboard[i];
            destroyEGLSurface(eglSurfaceWindows[i]);
            isurfaces[i]->Release();
        }

        idisplay->Release();
        termEGL();
        return (0);
    }

private:
    void drawFrame(EGLSurface eglSurfaceWindow) {

        eglMakeCurrent(
            eglDisplay, eglSurfaceWindow, eglSurfaceWindow, eglContext);

        GLclampf red = ((float)rand()/(float)(RAND_MAX)) * 1.0;
        GLclampf green = ((float)rand()/(float)(RAND_MAX)) * 1.0;
        GLclampf blue = ((float)rand()/(float)(RAND_MAX)) * 1.0;

        glClearColor(red, green, blue, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();

        eglSwapBuffers(eglDisplay, eglSurfaceWindow);
    }

    EGLSurface createEGLSurface(NativeWindowType native) {

        EGLSurface eglSurfaceWindow;
        eglSurfaceWindow = eglCreateWindowSurface(
                eglDisplay, eglConfig, native, NULL);
        return eglSurfaceWindow;
    }

    void destroyEGLSurface(EGLSurface eglSurfaceWindow) {

        eglDestroySurface(eglDisplay, eglSurfaceWindow);
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

    EGLDisplay eglDisplay;
    EGLConfig eglConfig;
    EGLContext eglContext;
    EGLSurface eglSurfaceWindows[MAX_SURFACES];

    Compositor::IDisplay *idisplay;
    Compositor::IDisplay::ISurface* isurfaces[MAX_SURFACES];
    Keyboard *ikeyboard[MAX_SURFACES];
};
} // WPEFramework

int main(int argc, char ** argv)
{
    srand(time(0));
    WPEFramework::TestContext *tcontext = new WPEFramework::TestContext();
    tcontext->run();
    return (0);
}
