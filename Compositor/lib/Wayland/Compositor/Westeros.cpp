#include "Module.h"
#include <westeros-compositor.h>
#include "Wayland.h"

#include <VirtualKeyboard.h>

namespace WPEFramework {
namespace Westeros {

    static const char* connectorName = "/tmp/keyhandler";

    class Compositor : public Implementation::IServer {
    private:
        Compositor() = delete;
        Compositor(const Compositor&) = delete;
        Compositor& operator=(const Compositor&) = delete;

    public:
        Compositor(const string& renderModule, const string& display, const uint32_t width, const uint32_t height)
            : _compositor(nullptr)
            , _virtualKeyboardHandle(nullptr)
        {
            TRACE(Trace::Information, (_T("Starting Compositor renderModule=%s display=%s"), renderModule.c_str(), display.c_str()));

            ASSERT(_instance == nullptr);
            _instance = this;

            string runtimeDir;

            Core::SystemInfo::GetEnvironment(_T("XDG_RUNTIME_DIR"), runtimeDir);

            Core::File path(runtimeDir);

            if (path.IsDirectory() == false) {
                Core::Directory(path.Name().c_str()).CreatePath();
                TRACE(Trace::Information, (_T("Created XDG_RUNTIME_DIR: %s\n"), path.Name().c_str()));
            }

            _compositor = WstCompositorCreate();

            ASSERT(_compositor != nullptr);

            if (_compositor != nullptr) {
                WstCompositorSetDisplayName(_compositor, display.c_str());
                WstCompositorSetRendererModule(_compositor, renderModule.c_str());
                WstCompositorSetOutputSize(_compositor, width, height);
                
                if (WstCompositorStart(_compositor) != true) {
                    TRACE(Trace::Information, (_T("Error Starting Compositor")));
                };

                ASSERT(_virtualKeyboardHandle == nullptr);

                if (_virtualKeyboardHandle == nullptr) {
                    TRACE(Trace::Information, (_T("Constructing virtual keyboard")));

                    const char* listenerName = "Westeros";
                    _virtualKeyboardHandle = Construct(listenerName, connectorName, VirtualKeyboardCallback);
                    if (_virtualKeyboardHandle == nullptr) {
                        TRACE(Trace::Information, (_T("Failed to construct virtual keyboard")));
                    }
                }
            }
        }

        virtual ~Compositor()
        {
            TRACE(Trace::Information, (_T("Destructing the compositor")));

            if (_compositor != nullptr) {
                WstCompositorStop(_compositor);
                WstCompositorDestroy(_compositor);
            }

            if (_virtualKeyboardHandle != nullptr) {
                Destruct(_virtualKeyboardHandle);
            }
        }

    public:
        void KeyEvent(const int keyCode, const unsigned int keyState, const unsigned int keyModifiers)
        {
            ASSERT(_compositor != nullptr);

            if (_compositor != nullptr) {
                TRACE(Trace::Information, (_T("Insert key into Westeros code=0x%04x, state=0x%04x, modifiers=0x%04x"), keyCode, keyState, keyModifiers));

                WstCompositorKeyEvent(_compositor, keyCode, keyState, keyModifiers);
            }
        }

        static Compositor* Create(const string& renderer, const string& display, uint32_t width, uint32_t height)
        {
            return _instance == nullptr ? new Westeros::Compositor(renderer, display, width, height) : _instance;
        }

        /*virtual*/ void SetInput(const char name[])
        {
            WstCompositorFocusClientByName(_compositor, name);
        }

    private:
        static void VirtualKeyboardCallback(actiontype type, unsigned int code)
        {
            TRACE_GLOBAL(Trace::Information, (_T("VirtualKeyboardCallback keycode 0x%04x is %s."),
                                                 code,
                                                 type == PRESSED ? "pressed" : type == RELEASED ? "released" : type == REPEAT ? "repeated" : type == COMPLETED ? "completed" : "unknown"));

            // TODO: no key repeat handled by westeros.

            int keyCode = code;
            unsigned int keyState;
            static unsigned int keyModifiers;

            switch (keyCode) {
            case KEY_LEFTSHIFT:
            case KEY_RIGHTSHIFT:
                TRACE_GLOBAL(Trace::Information, (_T("[ SHIFT ] was detected, current keyModifiers 0x%02x"), keyModifiers));
                if (type == PRESSED)
                    keyModifiers |= WstKeyboard_shift;
                else
                    keyModifiers &= ~WstKeyboard_shift;
                break;

            case KEY_LEFTCTRL:
            case KEY_RIGHTCTRL:
                TRACE_GLOBAL(Trace::Information, (_T("[ CTRL ] was detected, current keyModifiers 0x%02x"), keyModifiers));
                if (type == PRESSED)
                    keyModifiers |= WstKeyboard_ctrl;
                else
                    keyModifiers &= ~WstKeyboard_ctrl;
                break;

            case KEY_LEFTALT:
            case KEY_RIGHTALT:
                TRACE_GLOBAL(Trace::Information, (_T("[ ALT ] was detected, current keyModifiers 0x%02x"), keyModifiers));
                if (type == PRESSED)
                    keyModifiers |= WstKeyboard_alt;
                else
                    keyModifiers &= ~WstKeyboard_alt;
                break;
            default: {
                switch (type) {
                case RELEASED:
                    keyState = WstKeyboard_keyState_released;
                    break;
                case PRESSED:
                    keyState = WstKeyboard_keyState_depressed;
                    break;
                default:
                    keyState = WstKeyboard_keyState_none;
                    break;
                }

                if (keyState != WstKeyboard_keyState_none) {
                    _instance->KeyEvent(keyCode, keyState, keyModifiers);
                }
                break;
            }
            }
        }

    private:
        static void callback(bool ready)
        {
        }

    private:
        WstCompositor* _compositor;
        void* _virtualKeyboardHandle;
        static Westeros::Compositor* _instance;
    };

    /*static*/ Westeros::Compositor* Westeros::Compositor::_instance = nullptr;
} // namespace Westeros

extern "C" {
namespace Implementation {

    class Config : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
            : Core::JSON::Container()
            , Renderer(_T("/usr/lib/libwesteros_render_gl.so"))
            , Display(_T("wayland-0"))
        {
            Add(_T("renderer"), &Renderer);
            Add(_T("display"), &Display);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::String Renderer;
        Core::JSON::String Display;
        Core::JSON::DecUInt32 Width;
        Core::JSON::DecUInt32 Height;
    };

    IServer* Create(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        Config config;
        config.FromString(service->ConfigLine());

        return Westeros::Compositor::Create(config.Renderer.Value(), config.Display.Value(), config.Width.Value(), config.Height.Value());
    }
} // namespace Implementation
}
} // namespace WPEFramework
