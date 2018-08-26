#include "Implementation.h"
#include <virtualinput/virtualinput.h>

int g_pipefd[2];
struct Message {
    uint32_t code;
    actiontype type;
};

static const char * connectorName = "/tmp/keyhandler";
static void VirtualKeyboardCallback(actiontype type , unsigned int code) {
    if (type != COMPLETED) {
        Message message;
        message.code = code;
        message.type = type;
        write(g_pipefd[1], &message, sizeof(message));
    }
}

namespace WPEFramework {
namespace Rpi {

Display::SurfaceImplementation::SurfaceImplementation(
        Display& display, const std::string& name,
        const uint32_t width, const uint32_t height)
: _parent(display)
, _refcount(1)
, _name(name)
, _x(0)
, _y(0)
, _width(width)
, _height(height)
, _nativeWindow(nullptr)
, _keyboard(nullptr) {

    _parent.Register(this);
}

Display::SurfaceImplementation::~SurfaceImplementation() {
    _parent.Unregister(this);
}

Display::Display(const std::string& name)
: _displayName(name)
, _virtualkeyboard(nullptr)  {

    if (pipe(g_pipefd) == -1) {
        g_pipefd[0] = -1;
        g_pipefd[1] = -1;
    }

    _virtualkeyboard = Construct(
            name.c_str(), connectorName, VirtualKeyboardCallback);
    if (_virtualkeyboard == nullptr) {
        fprintf(stderr, "Initialization of virtual keyboard failed!!!\n");
    }
}

Display::~Display() {
    if (_virtualkeyboard != nullptr) {
        Destruct(_virtualkeyboard);
    }
}

int Display::Process(const uint32_t data) {
    Message message;
    if ((data != 0) && (g_pipefd[0] != -1) &&
            (read(g_pipefd[0], &message, sizeof(message)) > 0)) {

        std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());
        while (index != _surfaces.end()) {
            (*index)->SendKey(
                    message.code, (message.type == 0 ?
                            IDisplay::IKeyboard::released :
                            IDisplay::IKeyboard::pressed), time(nullptr));
            index++;
        }
    }
    return (0);
}

int Display::FileDescriptor() const {
    return (g_pipefd[0]);
}

Compositor::IDisplay::ISurface* Display::Create(
        const std::string& name, const uint32_t width, const uint32_t height) {
    return (new SurfaceImplementation(*this, name, width, height));
}

Compositor::IDisplay* Display::Instance(const std::string& displayName) {
    static Display myDisplay(displayName);
    return (&myDisplay);
}

}

Compositor::IDisplay* Compositor::IDisplay::Instance(
        const std::string& displayName) {
    return (Rpi::Display::Instance(displayName));
}
}
