#include "../Implementation.h"

#include <math.h>
#include <sys/time.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <wayland-client.h>
#include <linux/input.h>

#define Trace(fmt, args...) fprintf(stderr, "[%s:%d]: " fmt, __FILE__, __LINE__, ##args)

using namespace WPEFramework;

class GLContext : public Wayland::Display::IProcess {
private:
    GLContext() = delete;
    GLContext(const GLContext&) = delete;
    GLContext& operator=(const GLContext&) = delete;

public:
    GLContext(const Compositor::IDisplay::ISurface* surface)
        : _surface(surface)
        , _pos(0)
        , _col(1)
    {
        if (surface != nullptr) _surface->AddRef();
        _startTime = CurrentTime();
        _needRedraw = Initialize();
    }
    virtual ~GLContext()
    {
        if (surface != nullptr) _surface->Release();
    }

public:
    bool Dispatch();

private:
    bool Initialize();
    void Render();
    long long CurrentTime()
    {
        long long timeMillis;
        struct timeval tv;

        gettimeofday(&tv, nullptr);
        timeMillis = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        return timeMillis;
    }

private:
    bool _needRedraw;
    long long _startTime;
    GLuint _rotation_uniform;
    GLuint _pos;
    GLuint _col;
    Compositor::IDisplay:::ISurface _surface;
};

// ----------------------------------------------------------------------------------------
// GL Drawing code to prove the Wayland surface is properly configured.
// ----------------------------------------------------------------------------------------
static const char* vert_shader_text = "uniform mat4 rotation;\n"
                                      "attribute vec4 pos;\n"
                                      "attribute vec4 color;\n"
                                      "varying vec4 v_color;\n"
                                      "void main() {\n"
                                      "  gl_Position = rotation * pos;\n"
                                      "  v_color = color;\n"
                                      "}\n";

static const char* frag_shader_text = "precision mediump float;\n"
                                      "varying vec4 v_color;\n"
                                      "void main() {\n"
                                      "  gl_FragColor = v_color;\n"
                                      "}\n";

static GLuint createShader(GLenum shaderType, const char* shaderSource)
{
    GLuint shader = 0;
    GLint shaderStatus;
    GLsizei length;
    char logText[1000];

    shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, (const char**)&shaderSource, nullptr);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderStatus);
        if (!shaderStatus) {
            glGetShaderInfoLog(shader, sizeof(logText), &length, logText);
            Trace("Error compiling %s shader: %*s\n",
                ((shaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment"),
                length,
                logText);
        }
    }

    return shader;
}

bool GLContext::Initialize()
{
    bool result = false;
    GLuint frag, vert;
    GLuint program;
    GLint status;

    frag = createShader(GL_FRAGMENT_SHADER, frag_shader_text);
    vert = createShader(GL_VERTEX_SHADER, vert_shader_text);

    program = glCreateProgram();
    glAttachShader(program, frag);
    glAttachShader(program, vert);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char log[1000];
        GLsizei len;
        glGetProgramInfoLog(program, 1000, &len, log);
        Trace("Error: linking:\n%*s\n", len, log);
    }
    else {
        glUseProgram(program);

        glBindAttribLocation(program, _pos, "pos");
        glBindAttribLocation(program, _col, "color");
        glLinkProgram(program);

        _rotation_uniform = glGetUniformLocation(program, "rotation");
        result = true;
    }

    return result;
}

void GLContext::Render()
{
    static const GLfloat verts[3][2] = {
        { -0.5, -0.5 },
        { 0.5, -0.5 },
        { 0, 0.5 }
    };
    static const GLfloat colors[3][4] = {
        { 1, 0, 0, 1.0 },
        { 0, 1, 0, 1.0 },
        { 0, 0, 1, 1.0 }
    };
    GLfloat angle;
    GLfloat rotation[4][4] = {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    };
    static const uint32_t speed_div = 5;
    EGLint rect[4];

    glViewport(0, 0, _surface->Width(), _surface->Height());
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    angle = ((CurrentTime() - _startTime) / speed_div) % 360 * M_PI / 180.0;
    rotation[0][0] = cos(angle);
    rotation[0][2] = sin(angle);
    rotation[2][0] = -sin(angle);
    rotation[2][2] = cos(angle);

    glUniformMatrix4fv(_rotation_uniform, 1, GL_FALSE, static_cast<GLfloat*>(&(rotation[0][0])));

    glVertexAttribPointer(_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glVertexAttribPointer(_col, 4, GL_FLOAT, GL_FALSE, 0, colors);
    glEnableVertexAttribArray(_pos);
    glEnableVertexAttribArray(_col);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(_pos);
    glDisableVertexAttribArray(_col);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Trace("renderGL: glGetError() = %X\n", err);
    }
}

bool GLContext::Dispatch () {
   static struct wl_callback_listener frameListener =
{
        // redraw
        [](void* data, struct wl_callback* callback, uint32_t time) {
            GLContext& context = *reinterpret_cast<GLContext*>(data);

            // context._surface.Callback(nullptr);

            context._needRedraw = true;
        }
    };

    if (_needRedraw == true) {
        _needRedraw = false;
       Render();
        // _surface.Callback(&frameListener, this);
    }

    return (true);
}

static Compositor::IDisplay::ISurface* surfaceHandle = nullptr;

class Keyboard : public Wayland::Display::IKeyboard {
private:
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

public:
    Keyboard()
        : _modifiers(0)
        , opacityset(false)
    {
    }
    virtual ~Keyboard()
    {
    }

public:
    virtual void KeyMap(const char information[], const uint16_t size) override
    {
    }
    virtual void Key(const uint32_t key, const IKeyboard::state action, const uint32_t time) override
    {
        Trace("Received keycode %d was %s [modifiers 0x%02X]\n", key, action == 0 ? "released" : "pressed", _modifiers);

        if (action == IKeyboard::released) {
            switch (key) {
            case KEY_K:
                // kill
                Trace("[k] Sending Signal\n");
                Wayland::Display::Instance(std::string()).Signal();
                break;
            case KEY_O:
                //
                Trace("[o] set opacity\n");
                if (surfaceHandle != nullptr) {
                    if (opacityset) {
                        // surfaceHandle->Opacity(50);
                    }
                    else {
                        // surfaceHandle->Opacity(100);
                    }
                }
                break;
            case KEY_F:
                Trace("[f] Focus client\n");
                if (surfaceHandle != nullptr) {
                    // surfaceHandle->ZOrder(0);
                }
                break;
            default:
                Trace("[?] Unknown Command\n");
            }
        }
    }
    virtual void Modifiers(uint32_t depressedMods, uint32_t latchedMods, uint32_t lockedMods, uint32_t group) override
    {
        _modifiers = 0;
    }
    virtual void Repeat(int32_t rate, int32_t delay) override
    {
    }

private:
    uint32_t _modifiers;
    bool opacityset;
};

int main(int argc, const char* argv[])
{
    Wayland::Display& display(Wayland::Display::Instance(std::string()));

    display.InitializeEGL();

    Compositor::IDisplay::ISurface* client(display.Create("testClient", 1280, 720));

    GLContext process(client);
    Keyboard myKeyboard;

    client->Keyboard(&myKeyboard);

    if (display.IsOperational() == true) {

        // surfaceHandle = &client;

        // We also want to be aware of other surfaces.
        display.LoadSurfaces();

        // We need to trigger a first Draw.
        process.Dispatch();

        display.Process(&process);

        // surfaceHandle = nullptr;
    }

    client->Keyboard(nullptr);

    return 0;
}
