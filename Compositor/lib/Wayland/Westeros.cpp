/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "Wayland.h"
#include <westeros-compositor.h>
#include <westeros-gl.h>

#include <png.h>

namespace WPEFramework {
namespace Westeros {

    static const char* connectorName = "/tmp/keyhandler";

    class Compositor : public Implementation::IServer {
    private:
        Compositor(const string& renderModule, const string& display, const uint32_t width, const uint32_t height)
            : _compositor(nullptr)
            , _virtualInputHandle(nullptr)
            , _cursor(nullptr)
            , _xPos(0), _yPos(0)
            , _controller(nullptr)
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

                ASSERT(_virtualInputHandle == nullptr);

                const char* listenerName = "Westeros";
                if (_virtualInputHandle == nullptr) {
                    TRACE(Trace::Information, (_T("Constructing virtual keyboard")));

                    _virtualInputHandle = virtualinput_open(listenerName, connectorName, VirtualKeyboardCallback, VirtualMouseCallback, NULL /*callback for touch*/);
                    if (_virtualInputHandle == nullptr) {
                        TRACE(Trace::Information, (_T("Failed to construct virtual keyboard")));
                    }
                }
            }
        }

    public:
        Compositor() = delete;
        Compositor(const Compositor&) = delete;
        Compositor& operator=(const Compositor&) = delete;

        static Compositor* Create(const string& renderer, const string& display, uint32_t width, uint32_t height)
        {
            return _instance == nullptr ? new Westeros::Compositor(renderer, display, width, height) : _instance;
        }
        static Compositor* Instance() {
            return (_instance);
        }

        virtual ~Compositor()
        {
            TRACE(Trace::Information, (_T("Destructing the compositor")));

            if (_compositor != nullptr) {
                WstCompositorStop(_compositor);
                WstCompositorDestroy(_compositor);
            }
            if (_cursor) free(_cursor);

            if (_virtualInputHandle != nullptr) {
                virtualinput_close(_virtualInputHandle);
            }

            if (_controller != nullptr) {
                // Exit Wayland loop
                _controller->Signal();
                _controller->Release();
            }

            _instance = nullptr;
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

        void MouseClickEvent(const int keyCode, const unsigned int keyState)
        {
            ASSERT(_compositor != nullptr);

            if (_compositor != nullptr && _cursor != nullptr) {
                TRACE(Trace::Information, (_T("Westeros mouse code=0x%04x, state=0x%04x"), keyCode, keyState));
                WstCompositorPointerButtonEvent(_compositor, keyCode, keyState);
            }
        }

        void MouseMoveEvent(const int horizontal, const int vertical)
        {
            ASSERT(_compositor != nullptr);

            unsigned int width = 0, height = 0;
            if (_compositor != nullptr && _cursor != nullptr) {
                _xPos = _xPos + horizontal;
                _yPos = _yPos + vertical;

                WstCompositorGetOutputSize(_compositor, &width, &height);

                if (_xPos < 0) _xPos = 0;
                if (_xPos > (int)width) _xPos = width;
                if (_yPos < 0) _yPos = 0;
                if (_yPos > (int)height) _yPos = height;

                WstCompositorPointerMoveEvent(_compositor, _xPos, _yPos);
            }
        }

        void Get(const uint32_t id, Wayland::Display::Surface& surface) override
        {
            _controller->Get(id, surface);
        }

        void Process(Wayland::Display::IProcess* processloop) override
        {
            _controller->Process(processloop);
        }

        void SetInput(const string& name) override
        {
            WstCompositorFocusClientByName(_compositor, name.c_str());
        }

        bool StartController(const string& name, Wayland::Display::ICallback *callback) override
        {
            bool status = false;
            _controller = &(Wayland::Display::Instance(name));
            if (_controller != nullptr) {
                // OK ready to receive new connecting surfaces.
                _controller->Callback(callback);

                // We also want to be know the current surfaces.
                _controller->LoadSurfaces();
                status = true;
            }
            return status;
        }

    private:
        static void VirtualKeyboardCallback(keyactiontype type, unsigned int code)
        {
            TRACE_GLOBAL(Trace::Information, (_T("VirtualKeyboardCallback keycode 0x%04x is %s."), code, type == KEY_PRESSED ? "pressed" : type == KEY_RELEASED ? "released" : type == KEY_REPEAT ? "repeated" : type == KEY_COMPLETED ? "completed" : "unknown"));

            // TODO: no key repeat handled by westeros.

            int keyCode = code;
            unsigned int keyState;
            static unsigned int keyModifiers;

            switch (keyCode) {
            case KEY_LEFTSHIFT:
            case KEY_RIGHTSHIFT:
                TRACE_GLOBAL(Trace::Information, (_T("[ SHIFT ] was detected, current keyModifiers 0x%02x"), keyModifiers));
                if (type == KEY_PRESSED)
                    keyModifiers |= WstKeyboard_shift;
                else
                    keyModifiers &= ~WstKeyboard_shift;
                break;

            case KEY_LEFTCTRL:
            case KEY_RIGHTCTRL:
                TRACE_GLOBAL(Trace::Information, (_T("[ CTRL ] was detected, current keyModifiers 0x%02x"), keyModifiers));
                if (type == KEY_PRESSED)
                    keyModifiers |= WstKeyboard_ctrl;
                else
                    keyModifiers &= ~WstKeyboard_ctrl;
                break;

            case KEY_LEFTALT:
            case KEY_RIGHTALT:
                TRACE_GLOBAL(Trace::Information, (_T("[ ALT ] was detected, current keyModifiers 0x%02x"), keyModifiers));
                if (type == KEY_PRESSED)
                    keyModifiers |= WstKeyboard_alt;
                else
                    keyModifiers &= ~WstKeyboard_alt;
                break;
            default: {
                switch (type) {
                case KEY_RELEASED:
                    keyState = WstKeyboard_keyState_released;
                    break;
                case KEY_PRESSED:
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

        static void VirtualMouseCallback(mouseactiontype type, unsigned short button, short horizontal, short vertical)
        {
            unsigned int keyCode  = button;
            unsigned int keyState = WstKeyboard_keyState_none;

            TRACE_GLOBAL(Trace::Information, (_T("mouse event type %d, btn code %d\n"), type, button));

#define KEY_EVENT_CODE(btn, code) \
            switch (btn) { \
            case 0:  code = BTN_LEFT;   break; \
            case 1:  code = BTN_RIGHT;  break; \
            case 2:  code = BTN_MIDDLE; break; \
            default: code = button;     break; \
            } \

            switch (type) {
            case MOUSE_PRESSED:
                keyState = WstKeyboard_keyState_depressed;
                KEY_EVENT_CODE(button, keyCode);
                _instance->MouseClickEvent(keyCode, keyState);
                break;
            case MOUSE_RELEASED:
                keyState = WstKeyboard_keyState_released;
                KEY_EVENT_CODE(button, keyCode);
                _instance->MouseClickEvent(keyCode, keyState);
                break;
            case MOUSE_SCROLL:
                /* TODO - to handle mouse  scroll events */
                if (vertical == 1) { /* Scroll up */
                }
                else if (vertical == -1) { /* Scroll down */
                }
                break;
            case MOUSE_MOTION:
                _instance->MouseMoveEvent(horizontal, vertical);
                break;
            default:
                keyState= WstKeyboard_keyState_none;
                break;
            }
        }

    private:
        static void callback(bool ready)
        {
        }

/*
 * Copyright 2002-2010 Guillaume Cottenceau.
 * Licensed under the X11 license.
 */
        static unsigned char* ReadPNGFile(const char *filename, unsigned char &in_width, unsigned char &in_height)
        {
            FILE *fp = fopen(filename, "rb");
            if(!fp) return NULL;

            png_byte color_type;
            png_byte bit_depth;
            png_bytep *row_pointers = NULL;

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
            if(!png) return NULL;

            png_infop info = png_create_info_struct(png);
            if(!info) return NULL;

            if(setjmp(png_jmpbuf(png))) return NULL;

            png_init_io(png, fp);

            png_read_info(png, info);

            in_width   = png_get_image_width(png, info);
            in_height  = png_get_image_height(png, info);
            color_type = png_get_color_type(png, info);
            bit_depth  = png_get_bit_depth(png, info);

            // Read any color_type into 8bit depth, RGBA format.
            // See http://www.libpng.org/pub/png/libpng-manual.txt
            if(bit_depth == 16)
              png_set_strip_16(png);

            if(color_type == PNG_COLOR_TYPE_PALETTE)
              png_set_palette_to_rgb(png);

            // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
            if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
              png_set_expand_gray_1_2_4_to_8(png);

            if(png_get_valid(png, info, PNG_INFO_tRNS))
              png_set_tRNS_to_alpha(png);

            // These color_type don't have an alpha channel then fill it with 0xff.
            if(color_type == PNG_COLOR_TYPE_RGB ||
               color_type == PNG_COLOR_TYPE_GRAY ||
               color_type == PNG_COLOR_TYPE_PALETTE)
              png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

            if(color_type == PNG_COLOR_TYPE_GRAY ||
               color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
              png_set_gray_to_rgb(png);

            png_read_update_info(png, info);

            if (row_pointers) return NULL;

            row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * in_height);
            for(int y = 0; y < in_height; y++)
              row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));

            png_read_image(png, row_pointers);

            unsigned char* data = (unsigned char*)malloc(in_height * in_width * 4 /*each pixel in 4 bytes */);
            for(int y = 0, i = 0; y < in_height; y++) {
               memcpy(data + i, row_pointers[y], png_get_rowbytes(png,info));
               free(row_pointers[y]);
               i += png_get_rowbytes(png,info);
            }
            free(row_pointers);

            fclose(fp);
            png_destroy_read_struct(&png, &info, NULL);

            return data;
        }

    public:
        bool setCursor(const string& cursorFile)
        {
            if(_cursor) free(_cursor);

            unsigned char width = 0, height = 0;
            _cursor = ReadPNGFile(cursorFile.c_str(), width, height);
            if(!_cursor)
            {
                TRACE(Trace::Error, (_T("Unable to read given cursor PNG file %s"), cursorFile.c_str()));
                return false;
            }
            TRACE(Trace::Information, (_T("cursor: height %d, width %d\n"), height, width));

            if(!WstCompositorSetDefaultCursor( _compositor, _cursor, width, height, 0, 0 ))
            {
                const char *detail= WstCompositorGetLastErrorDetail(_compositor);
                TRACE(Trace::Error, (_T("Unable to set default cursor: error: (%s)\n"), detail));
                if(_cursor) free(_cursor);
                _cursor = nullptr;
                return false;
            }
            else
            {
                /* enabling pointer to use */
                WstCompositorPointerEnter(_compositor);
                TRACE(Trace::Information, (_T("mouse pointer cursor enabled")));
            }
            return true;
        }

    private:
        WstCompositor* _compositor;
        void* _virtualInputHandle;
        unsigned char* _cursor;
        int _xPos;
        int _yPos;
        Wayland::Display* _controller;
        Exchange::IComposition::ScreenResolution _resolution;
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
            , GLName(_T("libwesteros_gl.so.0.0.0"))
            , Display(_T("wayland-0"))
            , Cursor(_T(""))
        {
            Add(_T("renderer"), &Renderer);
            Add(_T("glname"), &GLName);
            Add(_T("display"), &Display);
            Add(_T("cursor"), &Cursor);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::String Renderer;
        Core::JSON::String GLName;
        Core::JSON::String Display;
        Core::JSON::String Cursor;
        Core::JSON::DecUInt32 Width;
        Core::JSON::DecUInt32 Height;
    };
 
    class WesterosGL {
    private:
        typedef bool (*SetDisplayMode)(WstGLCtx *ctx, const char *mode);
    public:
        WesterosGL(const WesterosGL&) = delete;
        WesterosGL& operator= (const WesterosGL&) = delete;

        WesterosGL()
            : _context(nullptr)
            , _resolution(Exchange::IComposition::ScreenResolution_1080i50Hz) {
        }
        ~WesterosGL() {
            if (_context != nullptr) {
                WstGLTerm(_context);
            }
        }

        void InitContext(const string& glName) {
            Core::Library library(glName.c_str());
            if (library.IsLoaded() == true) {
                _wstGLSetDisplayMode = reinterpret_cast<SetDisplayMode>(library.LoadFunction(_T("WstGLSetDisplayMode")));
                if (_wstGLSetDisplayMode != nullptr) {
                    _context = WstGLInit();
                }
            }
        }

    public:
        uint32_t SetResolution(const Exchange::IComposition::ScreenResolution value) {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_context && _wstGLSetDisplayMode) {

                const char* request = nullptr;
                switch(value) {
                    case Exchange::IComposition::ScreenResolution_480i:      request = "768x480";      break;
                    case Exchange::IComposition::ScreenResolution_480p:      request = "768x480";      break;
                    case Exchange::IComposition::ScreenResolution_720p:      request = "1280x720";     break;
                    case Exchange::IComposition::ScreenResolution_720p50Hz:  request = "1280x720x50";  break;
                    case Exchange::IComposition::ScreenResolution_1080p24Hz: request = "1920x1080x24"; break;
                    case Exchange::IComposition::ScreenResolution_1080i50Hz: request = "1920x1080x50"; break;
                    case Exchange::IComposition::ScreenResolution_1080p50Hz: request = "1920x1080x50"; break;
                    case Exchange::IComposition::ScreenResolution_1080p60Hz: request = "1920x1080x60"; break;
                    case Exchange::IComposition::ScreenResolution_2160p50Hz: request = "3840x2160x50"; break;
                    case Exchange::IComposition::ScreenResolution_2160p60Hz: request = "3840x2160x60"; break;
                    default: break;
                }
                if (request != nullptr) {
                    if (_wstGLSetDisplayMode(_context, request) == true) {
                        result = Core::ERROR_NONE;
                        _resolution = value;
                    } else {
                        result = Core::ERROR_GENERAL;
                    }
                } else {
                    result = Core::ERROR_UNKNOWN_KEY;
                }
            }

            return (result);
        }

        Exchange::IComposition::ScreenResolution GetResolution() const {
            return (_resolution);
        }

    private:
        WstGLCtx* _context;
        SetDisplayMode _wstGLSetDisplayMode;
        Exchange::IComposition::ScreenResolution _resolution;
    };

    static WesterosGL WesterosContext;

    uint32_t SetResolution(Exchange::IComposition::ScreenResolution value) {
        return (WesterosContext.SetResolution(value));
    }

    Exchange::IComposition::ScreenResolution GetResolution() {
        return (WesterosContext.GetResolution());
    }

    IServer* Create(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        Config config;
        config.FromString(service->ConfigLine());

        Westeros::Compositor* instance = Westeros::Compositor::Create(config.Renderer.Value(), config.Display.Value(), config.Width.Value(), config.Height.Value());
        string cursor = config.Cursor.Value();
        if(!cursor.empty()) {
            instance->setCursor(cursor);
        }

        WesterosContext.InitContext(config.GLName.Value());

        return instance;
    }
} // namespace Implementation
}
} // namespace WPEFramework
