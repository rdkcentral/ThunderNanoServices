/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#include <chrono>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <png.h>

#include <compositor/Client.h>

namespace WPEFramework {

class Keyboard : public Compositor::IDisplay::IKeyboard {
private:
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

public:
    Keyboard()
    {
    }
    virtual ~Keyboard()
    {
    }

public:
    virtual void AddRef() const override
    {
    }
    virtual uint32_t Release() const override
    {
        return 0;
    }
    virtual void KeyMap(const char information[], const uint16_t size) override
    {
    }
    virtual void Key(const uint32_t key,
        const IKeyboard::state action, const uint32_t time) override
    {
    }
    virtual void Modifiers(uint32_t depressedMods,
        uint32_t latchedMods, uint32_t lockedMods, uint32_t group) override
    {
    }
    virtual void Repeat(int32_t rate, int32_t delay) override
    {
    }
    virtual void Direct(const uint32_t key, const state action) override
    {
        fprintf(stderr, "KEY: %u for %s\n", key, _isurface->Name().c_str());
    }
    void SetSurface(Compositor::IDisplay::ISurface* surface)
    {
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

const char* vshader = R"(
       attribute vec4 position;
       attribute vec2 texcoord;
       varying vec2 texcoordVarying;
       void main() {
           gl_Position = position;
           texcoordVarying = texcoord;
       }
   )";

const char* fshader = R"(
       precision mediump float;
       varying vec2 texcoordVarying;
       uniform sampler2D texture;
       void main() {
           gl_FragColor = texture2D(texture, texcoordVarying);
       }
   )";

const float vertices[] = {
    -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f
};
const float texcoords[] = {
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f
};

#define MAX_SURFACES 3

bool mainloopRunning = true;
void intHandler(int)
{
    mainloopRunning = false;
}

class TestContext {

public:
    int run()
    {
        signal(SIGINT, intHandler);

        setupEGL();
        idisplay = Compositor::IDisplay::Instance(DisplayName());

        for (int i = 0; i < MAX_SURFACES; i++) {

            std::string name = "surface-" + std::to_string(i);
            isurfaces[i] = idisplay->Create(name, 1280, 720);
            eglSurfaceWindows[i] = createEGLSurface(isurfaces[i]->Native());
#if 0
            setupGL();
            drawImage(eglSurfaceWindows[i]);
            termGL();
#else
            drawFrame(eglSurfaceWindows[i]);
#endif
            ikeyboard[i] = new Keyboard();
            isurfaces[i]->Keyboard(ikeyboard[i]);
            ikeyboard[i]->SetSurface(isurfaces[i]);
        }

        while (mainloopRunning) {

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
    void setupGL()
    {

        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vshader, nullptr);
        glCompileShader(vertexShader);

        fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragShader, 1, &fshader, nullptr);
        glCompileShader(fragShader);

        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        position = glGetAttribLocation(program, "position");
        glEnableVertexAttribArray(position);
        texcoord = glGetAttribLocation(program, "texcoord");
        glEnableVertexAttribArray(texcoord);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    void termGL()
    {

        glDeleteTextures(1, &texture);

        glDetachShader(program, vertexShader);
        glDetachShader(program, fragShader);

        glDeleteProgram(program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragShader);
    }

    int loadPng(char* filename)
    {

        FILE* fp = fopen(filename, "rb");
        if (fp == nullptr) {
            printf("fopen failed\n");
            return (-1);
        }

        png_structp png = png_create_read_struct(
            PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        png_infop info = png_create_info_struct(png);

        png_init_io(png, fp);
        png_set_sig_bytes(png, 0);

        png_read_png(png, info, (PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND), nullptr);

        int bit_depth, color_type;
        png_get_IHDR(png, info, &imageWidth, &imageHeight,
            &bit_depth, &color_type, NULL, NULL, NULL);

        if (color_type == PNG_COLOR_TYPE_RGB) {
            imageFormat = GL_RGB;
        } else {
            imageFormat = GL_RGBA;
        }

        unsigned int row_bytes = png_get_rowbytes(png, info);
        imageData = (unsigned char*)malloc(row_bytes * imageHeight);

        png_bytepp rows = png_get_rows(png, info);
        for (unsigned int i = 0; i < imageHeight; ++i) {
            memcpy(imageData + (row_bytes * i), rows[i], row_bytes);
        }

        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return (0);
    }

    void deletePng()
    {
        free(imageData);
    }

    void drawImage(EGLSurface eglSurfaceWindow)
    {

        eglMakeCurrent(
            eglDisplay, eglSurfaceWindow, eglSurfaceWindow, eglContext);

        if (loadPng((char*)"/root/test.png") != 0) {

            GLclampf red = ((float)rand() / (float)(RAND_MAX)) * 1.0;
            GLclampf green = ((float)rand() / (float)(RAND_MAX)) * 1.0;
            GLclampf blue = ((float)rand() / (float)(RAND_MAX)) * 1.0;

            glClearColor(red, green, blue, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
        } else {

            glClearColor(0.0f, 0.0f, 0.0f, 0.1f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glTexImage2D(GL_TEXTURE_2D, 0, imageFormat,
                imageWidth, imageHeight,
                0, imageFormat, GL_UNSIGNED_BYTE, imageData);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glUseProgram(program);

            glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
            glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, vertices);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            deletePng();
        }
        eglSwapBuffers(eglDisplay, eglSurfaceWindow);
    }

    void drawFrame(EGLSurface eglSurfaceWindow)
    {

        eglMakeCurrent(
            eglDisplay, eglSurfaceWindow, eglSurfaceWindow, eglContext);

        GLclampf red = ((float)rand() / (float)(RAND_MAX)) * 1.0;
        GLclampf green = ((float)rand() / (float)(RAND_MAX)) * 1.0;
        GLclampf blue = ((float)rand() / (float)(RAND_MAX)) * 1.0;

        glClearColor(red, green, blue, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();

        eglSwapBuffers(eglDisplay, eglSurfaceWindow);
    }

    EGLSurface createEGLSurface(NativeWindowType native)
    {

        EGLSurface eglSurfaceWindow;
        eglSurfaceWindow = eglCreateWindowSurface(
            eglDisplay, eglConfig, native, NULL);
        eglMakeCurrent(
            eglDisplay, eglSurfaceWindow, eglSurfaceWindow, eglContext);
        return eglSurfaceWindow;
    }

    void destroyEGLSurface(EGLSurface eglSurfaceWindow)
    {

        eglDestroySurface(eglDisplay, eglSurfaceWindow);
    }

    void setupEGL()
    {

        eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(eglDisplay, NULL, NULL);

        EGLint configCount = 0;
        eglChooseConfig(eglDisplay, attrib_list, &eglConfig, 1, &configCount);
        eglContext = eglCreateContext(
            eglDisplay, eglConfig, EGL_NO_CONTEXT, context_attrib_list);
    }

    void termEGL()
    {

        eglDestroyContext(eglDisplay, eglContext);
        eglTerminate(eglDisplay);
    }

    std::string DisplayName()
    {
        std::string name;
        const char* callsign(std::getenv("CLIENT_IDENTIFIER"));

        if (callsign == nullptr) {
            name = "CompositorTest" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        } else {
            const char* delimiter = nullptr;
            if ((delimiter = strchr(callsign, ',')) == nullptr) {
                name = callsign;
            } else {
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

    GLuint imageWidth;
    GLuint imageHeight;
    unsigned char* imageData;
    GLenum imageFormat;

    GLuint program;
    GLuint vertexShader;
    GLuint fragShader;
    GLuint position;
    GLuint texcoord;
    GLuint texture;

    Compositor::IDisplay* idisplay;
    Compositor::IDisplay::ISurface* isurfaces[MAX_SURFACES];
    Keyboard* ikeyboard[MAX_SURFACES];
};
} // WPEFramework

int main(int argc, char** argv)
{
    srand(time(0));
    WPEFramework::TestContext* tcontext = new WPEFramework::TestContext();
    tcontext->run();
    return (0);
}
