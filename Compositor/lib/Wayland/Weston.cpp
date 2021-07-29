/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

extern "C" {
#include <libweston/libweston.h>
#include <libweston/backend.h>
#include <libweston/backend-drm.h>
#include <libweston/weston-log.h>
#include <libweston-desktop/libweston-desktop.h>
#include <weston/shared/helpers.h>

void weston_init_process_list();
void weston_compositor_shutdown(struct weston_compositor*);
}

namespace WPEFramework {
namespace Implementation {
namespace Weston {
    class Compositor : public Implementation::IServer, Core::Thread {
    private:
        enum KeyState {
            WestonKeyReleased = WL_KEYBOARD_KEY_STATE_RELEASED,
            WestonKeyPressed = WL_KEYBOARD_KEY_STATE_PRESSED,
            WestonKeyNone,
        };
        enum KeyModifiers {
            KeyShift = (1<<0),
            KeyAlt = (1<<1),
            KeyCtrl = (1<<2)
        };
    private:
        class Logger {
        private:
            class Config : public Core::JSON::Container {
            private:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

            public:
                Config()
                    : Core::JSON::Container()
                    , LogFileName(_T("weston.log"))
                    , LogFilePath(_T("/tmp"))
                {
                    Add(_T("logfilename"), &LogFileName);
                    Add(_T("logfilepath"), &LogFilePath);
                }
                ~Config() = default;

            public:
                Core::JSON::String LogFileName;
                Core::JSON::String LogFilePath;

            };

        private:
            static constexpr const uint32_t DefaultFlightRecSize = (5 * 1024 * 1024);
        public:
            Logger() = delete;
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&)= delete;
            Logger(PluginHost::IShell* service)
                : _context(nullptr)
                , _logger(nullptr)
                , _flightRec(nullptr)
            {
                Config config;
                config.FromString(service->ConfigLine());
                string logFilePath =
                    (config.LogFilePath.IsSet() == true) ?
                    Core::Directory::Normalize(config.LogFilePath.Value()) : service->VolatilePath();
                CreateLogFile(string(logFilePath + config.LogFileName.Value()));

                _context = weston_log_ctx_compositor_create();
                ASSERT(_context);
                if (_context) {
                    _logScope = weston_compositor_add_log_scope(_context, "log",
                                "Weston and Wayland logger\n", nullptr, nullptr, nullptr);

                    SetHandler();
                    SubscribeLogger();
                }
            }
            ~Logger() {
                UnSubscribeLogger();
                _logFile.Close();
            }
            void DestroyLogScope()
            {
                weston_compositor_log_scope_destroy(_logScope);
                _logScope = nullptr;
            }
            struct weston_log_context* Context() const
            {
                return _context;
            }

        private:
            static void CustomHandler(const char *format, va_list argument)
            {
                Core::Time now(Core::Time::Now());
                string timeStamp(now.ToRFC1123(true));
                weston_log_scope_printf(_logScope, "%s libwayland: ", timeStamp.c_str());
                weston_log_scope_vprintf(_logScope, format, argument);
            }

            void CreateLogFile(const string& logName)
            {
                wl_log_set_handler_server(CustomHandler);
                _logFile = Core::File(logName);
                if (_logFile.Append() == true) {
                    FILE* handle = _logFile;
                    if (handle != nullptr) {
                        int fd = fileno(handle);
                        int flags = fcntl(fd, F_GETFD);
                        if (flags >= 0) {
                            fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
                        }
                    }
                }
            }
            inline void SetHandler()
            {
                if (_logFile != nullptr) {
                    weston_log_set_handler(Log, LogContinue);
                }
            }
            inline void SubscribeLogger()
            {
                _logger = weston_log_subscriber_create_log(_logFile);
                weston_log_subscribe(_context, _logger, "log");

                _flightRec = weston_log_subscriber_create_flight_rec(DefaultFlightRecSize);
                weston_log_subscribe(_context, _flightRec, "log");
                weston_log_subscribe(_context, _flightRec, "drm-backend");
            }
            inline void UnSubscribeLogger()
            {
                weston_log_subscriber_destroy_log(_logger);
                weston_log_subscriber_destroy_flight_rec(_flightRec);
            }
            static int Log(const string& timeStamp, const char* format, va_list argp)
            {
                int length = 0;

                if (weston_log_scope_is_enabled(_logScope)) {
                    char* str;
                    length = vasprintf(&str, format, argp);
                    if (length >= 0) {
                        length = weston_log_scope_printf(_logScope, "%s%s", timeStamp.c_str(), str);
                        free(str);
                    } else {
                        length = weston_log_scope_printf(_logScope, "%sOut of memory", timeStamp.c_str());
                    }
                    fflush(0);
                }
                return length;
            }
            static int LogContinue(const char* format, va_list argp)
            {
                return Log("", format, argp);
            }
            static int Log(const char* format, va_list argp)
            {
                Core::Time now(Core::Time::Now());
                string timeStamp = now.ToRFC1123(true) + " ";

                return Log(timeStamp, format, argp);
            }
        private:
            struct weston_log_context* _context;
            struct weston_log_subscriber* _logger;
            struct weston_log_subscriber* _flightRec;
            static struct weston_log_scope* _logScope;

            static Core::File _logFile;
        };

    private:
        class InputController {
        private:
            struct KeyData {
                uint32_t Code;
                uint32_t Modifiers;
                wl_keyboard_key_state State;
                struct wl_event_source* Timer;
                InputController* Parent;
            };
            static constexpr const char* connectorName = "/tmp/keyhandler";

        public:
            InputController() = delete;
            InputController(const  InputController&) = delete;
            InputController& operator=(const InputController&) = delete;

            InputController(Compositor* parent)
                : _parent(parent)
                , _keys()
                , _seats()
                , _virtualInputHandle(nullptr)
            {

                if (_instance == nullptr) {
                    _instance = this;
                }

                ASSERT(_virtualInputHandle == nullptr);
                const char* listenerName = "Weston";
                if (_virtualInputHandle == nullptr) {
                    TRACE(Trace::Information, (_T("Constructing virtual keyboard")));

                    _virtualInputHandle = virtualinput_open(listenerName, connectorName, VirtualKeyboardCallback, nullptr /*callback for mouse*/, nullptr /*callback for touch*/);
                    if (_virtualInputHandle == nullptr) {
                        TRACE(Trace::Information, (_T("Failed to construct virtual keyboard")));
                    }
                }
            }
            ~InputController()
            {
                FlushInputs();
                if (_virtualInputHandle != nullptr) {
                    virtualinput_close(_virtualInputHandle);
                }
                _instance = nullptr;
            }

        public:
            inline void CollectInputHandlers()
            {
                struct weston_seat* seat;
                struct weston_compositor* compositor = _parent->PlatformCompositor();
                wl_list_for_each(seat, &compositor->seat_list, link) {
                    _seats.push_back(seat);
                }
            }
            inline void SetInput(struct weston_surface* surface) {
                for (const auto& seat: _seats) {
                    weston_seat_set_keyboard_focus(seat, surface);
                }
            }

        private:
            inline std::list<struct weston_seat*>& Seats()
            {
                return _seats;
            }
            inline void RemoveKey(KeyData* key)
            {
                _keys.remove(key);
            }
            inline void FlushInputs()
            {
                for (auto& key: _keys) {
                    wl_event_source_remove(key->Timer);
                    _keys.remove(key);
                     delete key;
                }
                _seats.clear();
            }
            void KeyEvent(uint32_t keyCode, wl_keyboard_key_state keyState, const unsigned int keyModifiers)
            {
                struct weston_compositor* compositor = _parent->PlatformCompositor();

                if (compositor != nullptr) {
                    TRACE(Trace::Information, (_T("Insert key into Westeros code=0x%04x, state=0x%04x, modifiers=0x%04x"), keyCode, keyState, keyModifiers));
                    struct wl_event_loop* loop = wl_display_get_event_loop(compositor->wl_display);
                    KeyData* key = new KeyData();
                    key->Code = keyCode;
                    key->Modifiers = keyModifiers;
                    key->State = keyState;
                    key->Parent = this;
                    key->Timer = wl_event_loop_add_timer(loop, SendKey, key);
                    wl_event_source_timer_update(key->Timer, 1);

                    _keys.push_back(key);
                }
            }
            static int SendKey(void* data) {
                KeyData* key = static_cast<KeyData*>(data);
                InputController* parent = key->Parent;

                for (const auto& seat: parent->Seats()) {
                    SendModifiers(seat, key->Modifiers);
                    struct timespec time = { 0 };
                    weston_compositor_get_time(&time);
                    notify_key(seat, &time, key->Code, key->State, STATE_UPDATE_AUTOMATIC);
                }
                wl_event_source_remove(key->Timer);

                parent->RemoveKey(key);
                delete key;

                return 1;
            }
            static void SendModifiers(struct weston_seat *seat, uint32_t modifiers)
            {
                struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
                if (keyboard) {
                    xkb_mod_mask_t depressed = 0;
                    xkb_mod_mask_t latched = xkb_state_serialize_mods(keyboard->xkb_state.state,
                                             static_cast<xkb_state_component>(XKB_STATE_LATCHED));
                    xkb_mod_mask_t locked = xkb_state_serialize_mods(keyboard->xkb_state.state,
                                             static_cast<xkb_state_component>(XKB_STATE_LOCKED));
                    xkb_mod_mask_t group = xkb_state_serialize_group(keyboard->xkb_state.state,
                                             static_cast<xkb_state_component>(XKB_STATE_EFFECTIVE));

                    if (modifiers & KeyModifiers::KeyShift) {
                        depressed |= (1 << keyboard->xkb_info->shift_mod);
                    }
                    if (modifiers & KeyModifiers::KeyAlt) {
                        depressed |= (1 << keyboard->xkb_info->alt_mod);
                    }
                    if (modifiers & KeyModifiers::KeyCtrl) {
                        depressed |= (1 << keyboard->xkb_info->ctrl_mod);
                    }
                    xkb_state_update_mask(keyboard->xkb_state.state, depressed, latched, locked, 0, 0, group);
                }
            }
            static void VirtualKeyboardCallback(keyactiontype type, unsigned int code)
            {
                TRACE_GLOBAL(Trace::Information, (_T("VirtualKeyboardCallback keycode 0x%04x is %s."), code, type == KEY_PRESSED ? "pressed" : type == KEY_RELEASED ? "released" : type == KEY_REPEAT ? "repeated" : type == KEY_COMPLETED ? "completed" : "unknown"));

                uint32_t keyCode = static_cast<uint32_t>(code);
                KeyState keyState;
                static uint32_t keyModifiers = 0;

                switch (keyCode) {
                case KEY_LEFTSHIFT:
                case KEY_RIGHTSHIFT:
                    TRACE_GLOBAL(Trace::Information, (_T("[ SHIFT ] was detected, current keyModifiers 0x%02x"), keyModifiers));
                    if (type == KEY_PRESSED)
                        keyModifiers |= KeyModifiers::KeyShift;
                    else
                       keyModifiers &= ~KeyModifiers::KeyShift;
                    break;

                case KEY_LEFTCTRL:
                case KEY_RIGHTCTRL:
                    TRACE_GLOBAL(Trace::Information, (_T("[ CTRL ] was detected, current keyModifiers 0x%02x"), keyModifiers));
                    if (type == KEY_PRESSED)
                        keyModifiers |= KeyModifiers::KeyCtrl;
                    else
                        keyModifiers &= ~KeyModifiers::KeyCtrl;
                    break;

                case KEY_LEFTALT:
                case KEY_RIGHTALT:
                    TRACE_GLOBAL(Trace::Information, (_T("[ ALT ] was detected, current keyModifiers 0x%02x"), keyModifiers));
                    if (type == KEY_PRESSED)
                        keyModifiers |= KeyModifiers::KeyAlt;
                    else
                        keyModifiers &= ~KeyModifiers::KeyAlt;
                    break;

                default: {
                    switch (type) {
                    case KEY_RELEASED:
                        keyState = KeyState::WestonKeyReleased;
                        break;
                    case KEY_PRESSED:
                        keyState = KeyState::WestonKeyPressed;
                        break;
                    default:
                        keyState = KeyState::WestonKeyNone;
                        break;
                    }

                    if (keyState != KeyState::WestonKeyNone) {
                        _instance->KeyEvent(keyCode, static_cast<wl_keyboard_key_state>(keyState), keyModifiers);
                    }
                    break;
                }
                }
            }

        private:
            Compositor* _parent;
            std::list<KeyData*> _keys;
            std::list<struct weston_seat*> _seats;
            void* _virtualInputHandle;
            static InputController* _instance;
        };

    private:
        class SurfaceData : public WPEFramework::Compositor::IDisplay::ISurface {
        public:
            SurfaceData() = delete;
            SurfaceData(const SurfaceData&) = delete;
            SurfaceData& operator=(const SurfaceData&) = delete;

            SurfaceData(Compositor* parent, const uint32_t id, const string& name, struct weston_surface* surface)
                : _parent(parent)
                , _id(id)
                , _name(name)
                , _refCount(0)
                , _timer(nullptr)
                , _surface(surface)
            {
                RegisterSurfaceDestroyListener(surface);
            }
            virtual ~SurfaceData()
            {
                RemoveTimer();
            }

        public:
            uint32_t Id() const
            {
                return _id;
            }
            void AddRef() const override
            {
                Core::InterlockedIncrement(_refCount);
            }
            uint32_t Release() const override
            {
                uint32_t result = Core::ERROR_NONE;
                if (Core::InterlockedDecrement(_refCount) == 0) {
                    delete this;
                    result = Core::ERROR_DESTRUCTION_SUCCEEDED;
                }
                return (result);
            }
            EGLNativeWindowType Native() const override
            {
                return 0;
            }
            std::string Name() const override
            {
                return _name;
            }
            int32_t Height() const override
            {
                return (_height);
            }
            int32_t Width() const override
            {
                return (_width);
            }
            static int SetOrder(void* data)
            {
                SurfaceData* surfaceData = static_cast<SurfaceData*>(data);
                if (!surfaceData->ZOrder()) {
                    struct weston_surface* surface = surfaceData->WestonSurface();
                    if (surface) {
                        struct weston_desktop_surface *desktop_surface = weston_surface_get_desktop_surface(surface);
                        if (desktop_surface) {

                            weston_desktop_surface_get_activated(desktop_surface);
                            surfaceData->WestonSurfaceSetTopAPI();
                        }
                    }
                }
                surfaceData->RemoveTimer();
                return 1;
            }
            void ZOrder(const uint32_t zorder) override
            {
                if (_timer != nullptr) {
                    wl_event_source_remove(_timer);
                }
                _zorder = zorder;
                struct weston_compositor* compositor = _parent->PlatformCompositor();
                struct wl_event_loop* loop = wl_display_get_event_loop(compositor->wl_display);
                _timer = wl_event_loop_add_timer(loop, SetOrder, this);
                wl_event_source_timer_update(_timer, 1);
            }
            inline void RemoveTimer()
            {
                if (_timer) {
                    wl_event_source_remove(_timer);
                    _timer = nullptr;
                }
            }
            inline uint32_t ZOrder() const
            {
                return _zorder;
            }
            inline struct weston_compositor* WestonCompositor() const
            {
                return _parent->PlatformCompositor();
            }
            inline struct weston_surface* WestonSurface() const
            {
                return _surface;
            }
            inline void WestonSurfaceSetTopAPI() const
            {
                if (_parent->_surfaceSetTopAPI) {
                    _parent->_surfaceSetTopAPI(_surface);
                }
            }
            inline void RemoveSurface() const
            {
                _parent->RemoveSurface(_surface);
            }
            inline void RegisterSurfaceDestroyListener(struct weston_surface* surface) {
                _surfaceDestroyListener.notify = NotifySurfaceDestroy;
                wl_signal_add(&surface->destroy_signal, &_surfaceDestroyListener);
            }
            static void NotifySurfaceDestroy(struct wl_listener* listener, void* data)
            {
                SurfaceData* surfaceData;
                surfaceData = wl_container_of(listener, surfaceData, _surfaceDestroyListener);
                surfaceData->RemoveSurface();
            }

        private:
            Compositor* _parent;

            uint32_t _id;
            int32_t _width;
            int32_t _height;
            uint32_t _zorder;
            std::string _name;
            mutable uint32_t _refCount;
            struct wl_event_source* _timer;
            struct weston_surface* _surface;
            struct wl_listener _surfaceDestroyListener;
        };
        typedef std::map<const string, SurfaceData*> SurfaceMap;
        typedef void (*ShellSurfaceSetTop)(struct weston_surface*);

    private:
        class IBackend {
        private:
            class Connector : public Core::JSON::Container {
            private:
                Connector& operator=(const Connector&) = delete;

            public:
                Connector()
                    : Core::JSON::Container()
                    , Name("")
                    , Mode("preferred")
                    , Transform("normal")
                {
                    Add(_T("name"), &Name);
                    Add(_T("mode"), &Mode);
                    Add(_T("transform"), &Transform);
                }
                Connector(const Connector& copy)
                    : Core::JSON::Container()
                    , Name(copy.Name)
                    , Mode(copy.Mode)
                    , Transform(copy.Transform)
                {
                    Add(_T("name"), &Name);
                    Add(_T("mode"), &Mode);
                    Add(_T("transform"), &Transform);
                }
                ~Connector() = default;
            public:
                Core::JSON::String Name;
                Core::JSON::String Mode;
                Core::JSON::String Transform;
            };
            class Config : public Core::JSON::Container {
            private:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

            public:
                Config()
                    : Core::JSON::Container()
                    , Forced(false)
                    , Connectors()
                {
                    Add(_T("forced"), &Forced);
                    Add(_T("outputs"), &Connectors);
                }
                ~Config() = default;

            public:
                Core::JSON::Boolean Forced;
                Core::JSON::ArrayType<Connector> Connectors;
            };

        public:
            class Output {
            public:
                Output()
                    : _name()
                    , _mode()
                    , _transform()
                {
                }
                Output(const string& name, const string& mode, const string& transform)
                    : _name(name)
                    , _mode(mode)
                    , _transform(transform)
                {

                }
                Output(const Output& copy)
                    : _name(copy._name)
                    , _mode(copy._mode)
                    , _transform(copy._transform)
                {
                }
                Output& operator=(const Output& ref)
                {
                    _name = ref._name;
                    _mode = ref._mode;
                    _transform = ref._transform;
                    return (*this);
                }
                ~Output() = default;
            public:
                string Name() const
                {
                    return _name;
                }
                string Mode() const
                {
                    return _mode;
                }
                string Transform() const
                {
                    return _transform;
                }

            private:
                string _name;
                string _mode;
                string _transform;
            };
        public:
            struct Layoutput {
                string Name;
                struct wl_list CompositorLink;
                struct wl_list OutputList;
                std::list<struct weston_head*> Clones;
                Output Config;
            };
            struct OutputData {
                struct weston_output* Output;
                struct wl_listener DestroyListener;
                struct Layoutput* Layoutput;
                struct wl_list Link;
            };

        public:
            IBackend(const IBackend&) = delete;
            IBackend& operator=(const IBackend&)= delete;
            IBackend() = delete;
            IBackend(PluginHost::IShell* service)
                : _forced(false)
                , _outputs()
            {
                Config config;
                config.FromString(service->ConfigLine());

                auto index(config.Connectors.Elements());
                while (index.Next() == true) {
                    if (index.Current().Name.IsSet() == true) {
                        _outputs.emplace(std::piecewise_construct,
                        std::make_tuple(index.Current().Name.Value()),
                        std::make_tuple(
                            index.Current().Name.Value(),
                            index.Current().Mode.Value(),
                            index.Current().Transform.Value()));
                    }
                }

                _forced = config.Forced.Value();
            }
            virtual ~IBackend() {
                _outputs.clear();
            }

        protected:
            void DestroyOutput(OutputData* output)
            {
                if (output->Output) {
                    struct weston_output* save = output->Output;
                    OutputHandleDestroy(&output->DestroyListener, save);
                    weston_output_destroy(save);
                }

                wl_list_remove(&output->Link);
                free(output);
            }
            static void OutputHandleDestroy(struct wl_listener* listener, void* data)
            {
                OutputData* outputData;
                outputData = wl_container_of(listener, outputData, DestroyListener);
                assert(outputData->Output == static_cast<struct weston_output*>(data));
                outputData->Output = nullptr;
                wl_list_remove(&outputData->DestroyListener.link);
            }
        public:
            bool Forced() const
            {
                return _forced;
            }
            Output OutputConfig(const string& name)
            {
                Output output;
                std::map<const string, Output>::iterator index(_outputs.find(name));
                if (index != _outputs.end()) {
                    output = index->second;
                }
                return output;
            }
            virtual void Load(Compositor*) = 0;
            virtual uint32_t UpdateResolution(const string&) = 0;
        protected:
            bool _forced;
            std::map<const string, Output> _outputs;
            struct wl_list _layoutputList;
        };

     private:
        class DRM : public IBackend{
        private:
            class Config : public Core::JSON::Container {
            private:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

            public:
                Config()
                    : Core::JSON::Container()
                    , TTY()
                {
                    Add(_T("tty"), &TTY);
                }
                ~Config()
                {
                }

            public:
                Core::JSON::ArrayType<Core::JSON::DecUInt8> TTY;
            };
        public:
            struct HeadTracker {
                struct wl_listener HeadDestroyListener;
            };

        public:
            DRM() = delete;
            DRM(const DRM&) = delete;
            DRM& operator=(const DRM&)= delete;
            DRM(PluginHost::IShell* service)
                : IBackend(service)
                , _tty()
                , _listener()
                , _parent(nullptr)
            {
                TRACE(Trace::Information, (_T("Starting DRM Backend")));
                Config config;
                config.FromString(service->ConfigLine());

                auto index(config.TTY.Elements());
                while (index.Next() == true) {
                    if (index.Current().IsSet() == true) {
                        _tty.push_back(index.Current().Value());
                    }
                }

                wl_list_init(&_layoutputList);
            }
            ~DRM() override
            {
                DestroyLayoutputList();
            }

        public:
            void Load(Compositor* parent) override
            {
                TRACE(Trace::Information, (_T("Loading DRM Backend")));
                struct weston_drm_backend_config config = {{ 0 }};

                config.base.struct_version = WESTON_DRM_BACKEND_CONFIG_VERSION;
                config.base.struct_size = sizeof(struct weston_drm_backend_config);

                _listener.notify = HeadsChanged;
                _parent = parent;
                struct weston_compositor* compositor = parent->PlatformCompositor();
                weston_compositor_add_heads_changed_listener(compositor, &_listener);

                auto tty = _tty.begin();
                for (;tty != _tty.end();++tty) {
                    config.tty = *tty;
                    if (weston_compositor_load_backend(compositor, WESTON_BACKEND_DRM, &config.base) >= 0) {
                        break;
                    }
                }

                if (tty == _tty.end()) {
                    weston_compositor_shutdown(compositor);
                    TRACE(Trace::Error, (_T("Failed to load DRM backend using all configured tty")));
                }
            }
        private:
            static uint16_t CountRemainingHeads(struct weston_output* output, struct weston_head* remaining)
            {
                uint16_t count = 0;
                struct weston_head* index = nullptr;
                while ((index = weston_output_iterate_heads(output, index)) != nullptr) {
                    if (index != remaining) {
                        count++;
                    }
                }
                return count;
            }
            void CreateHeadTracker(struct weston_head* head)
            {
                HeadTracker* tracker = new HeadTracker();
                ASSERT(tracker);
                if (tracker) {
                    tracker->HeadDestroyListener.notify = HandleHeadDestroy;
                    weston_head_add_destroy_listener(head, &tracker->HeadDestroyListener);
                }
            }
            HeadTracker* GetHeadTracker(struct weston_head* head)
            {
                HeadTracker* tracker = nullptr;
                struct wl_listener* listener = weston_head_get_destroy_listener(head, HandleHeadDestroy);
                if (listener) {
                    tracker = container_of(listener, HeadTracker, HeadDestroyListener);
                }
                return tracker;
            }
            static void DestroyHeadTracker(HeadTracker* track)
            {
                wl_list_remove(&track->HeadDestroyListener.link);
                delete(track);
            }
            Layoutput* FindLayoutput(const string& name)
            {
                Layoutput* layoutput = nullptr;
                Layoutput* tmpLayoutput = nullptr;

                wl_list_for_each(tmpLayoutput, &_layoutputList, CompositorLink) {
                    if (layoutput->Name == name) {
                        layoutput = tmpLayoutput;
                        break;
                    }
                }
                return layoutput;
            }
            Layoutput* CreateLayoutput(const string& name, const IBackend::Output& output)
            {
                Layoutput* layoutput = new Layoutput();
                ASSERT(layoutput);
                if (layoutput) {
                    wl_list_insert(_layoutputList.prev, &layoutput->CompositorLink);
                    wl_list_init(&layoutput->OutputList);
                    layoutput->Name = name;
                    layoutput->Config = output;
                }
                return layoutput;
            }
            void DestroyLayoutputList()
            {
                Layoutput* layoutput;
                Layoutput* templayoutput;
                IBackend::OutputData* output;
                IBackend::OutputData* tempOutput;

                wl_list_for_each_safe(layoutput,  templayoutput, &_layoutputList, CompositorLink) {
                    wl_list_for_each_safe(output, tempOutput, &layoutput->OutputList, Link) {
                        DestroyOutput(output);
                    }
                        wl_list_remove(&layoutput->CompositorLink);
                    delete layoutput;
                }
            }
            void LayoutputAddHead(struct weston_head* head, const IBackend::Output& output)
            {
                const char* outputName = weston_head_get_name(head);
                Layoutput* layoutput = FindLayoutput(outputName);
                if (layoutput == nullptr) {
                    layoutput = CreateLayoutput(outputName, output);
                }
                if (layoutput->Clones.size() < MaxCloneHeads) {
                    layoutput->Clones.push_back(head);
                }
                return;
            }
            void HeadPrepareEnable(struct weston_head* head)
            {
                 const char* outputName = weston_head_get_name(head);
                 Output output = OutputConfig(outputName);
                 LayoutputAddHead(head, output);
            }
            static void HandleHeadDestroy(struct wl_listener* listener, void* data)
            {
                struct weston_head* head = static_cast<struct weston_head*>(data);
                HeadTracker* track = container_of(listener, HeadTracker, HeadDestroyListener);

                DestroyHeadTracker(track);

                struct weston_output* output = weston_head_get_output(head);

                if (output) {
                    if (CountRemainingHeads(output, head) <= 0) {
                        weston_output_destroy(output);
                    }
                }
                return;
            }
            uint32_t ConfigureOutput(struct weston_output* output, const IBackend::Output& config)
            {
                uint32_t status = Core::ERROR_NONE;
                const struct weston_drm_output_api* api = weston_drm_output_get_api(_parent->PlatformCompositor());

                if (api) {
                    if (!api->set_mode(output, WESTON_DRM_BACKEND_OUTPUT_PREFERRED, config.Mode().c_str())) {
                        api->set_gbm_format(output, NULL);
                        api->set_seat(output, "");
                        weston_output_set_scale(output, 1);
                        uint32_t transform = 0;
                        weston_parse_transform(config.Transform().c_str(), &transform);
                        weston_output_set_transform(output, transform);
                        weston_output_allow_protection(output, true);

                    } else {
                        weston_log("Cannot configure an output using weston_drm_output_api.\n");
                        status = Core::ERROR_NOT_SUPPORTED;
                    }

                } else {
                    weston_log("Cannot use weston_drm_output_api.\n");
                    status = Core::ERROR_NOT_SUPPORTED;
                }

                return status;

            }
            OutputData* GetOutputFromWeston(struct weston_output* base)
            {
                OutputData* output = nullptr;
                struct wl_listener* listener = weston_output_get_destroy_listener(base, OutputHandleDestroy);
                if (listener) {
                    output = container_of(listener, OutputData, DestroyListener);
                }
                return output;
            }
            void TryAttach(struct weston_output* output, std::list<struct weston_head*>& Clones, std::list<struct weston_head*>& failedClones) {
                for (std::list<struct weston_head*>::iterator clone = Clones.begin(); clone != Clones.end(); clone++) {
                    if (weston_output_attach_head(output, *clone) < 0) {
                        Clones.erase(clone);
                        failedClones.push_back(*clone);
                    }
                }
            }
            uint32_t TryEnable(struct weston_output* output, std::list<struct weston_head*>& Clones, std::list<struct weston_head*>& failedClones) {
                uint32_t status = Core::ERROR_GENERAL;
                std::list<struct weston_head*>::reverse_iterator clone = Clones.rbegin();
                while (!output->enabled && (clone != Clones.rend())) {
                    if (weston_output_enable(output) == 0) {
                        status = Core::ERROR_NONE;
                        break;
                    }

                    weston_head_detach(*clone);
                    failedClones.push_back(*clone);
                    Clones.erase(std::next(clone).base());
                }
                return status;
            }
            uint32_t TryAttachEnable(struct weston_output* output, IBackend::Layoutput* layoutput)
            {
                std::list<struct weston_head*> failedClones;
                uint32_t status = Core::ERROR_NONE;

                TryAttach(output, layoutput->Clones, failedClones);
                status = ConfigureOutput(output, layoutput->Config);
                if (status == Core::ERROR_NONE) {

                    status = TryEnable(output, layoutput->Clones, failedClones);
                    if (status == Core::ERROR_NONE) {

                        /* For all successfully attached/enabled heads */
                        for (auto& clone : layoutput->Clones) {
                            CreateHeadTracker(clone);
                        }
                        /* Push failed heads to the next round. */
                        layoutput->Clones.clear();
                        layoutput->Clones = failedClones;
                    }
                }
                return status;
            }
            struct OutputData* LayoutputCreateOutput(IBackend::Layoutput* layoutput, const string& name)
            {
                OutputData* output = new OutputData();

                ASSERT(output);
                if (output) {
                    output->Output = weston_compositor_create_output(_parent->PlatformCompositor(), name.c_str());
                    ASSERT(output->Output);
                    if (output->Output) {
                        output->Layoutput = layoutput;
                        wl_list_insert(layoutput->OutputList.prev, &output->Link);
                        output->DestroyListener.notify = OutputHandleDestroy;
                        weston_output_add_destroy_listener(output->Output, &output->DestroyListener);
                    } else {
                       delete output;
                       output = nullptr;
                    }
                }
                return output;
            }
            uint32_t ProcessLayoutput(IBackend::Layoutput* layoutput)
            {
                uint32_t status = Core::ERROR_NOT_SUPPORTED;
                IBackend::OutputData* tmp;
                IBackend::OutputData* output;

                wl_list_for_each_safe(output, tmp, &layoutput->OutputList, Link) {
                    std::list<struct weston_head*> failedClones;

                    if (!output->Output) {
                        /* Clean up left-overs from destroyed heads. */
                        DestroyOutput(output);
                        continue;
                    }

                    TryAttach(output->Output, layoutput->Clones, failedClones);
                    layoutput->Clones = failedClones;
                    if (failedClones.size() == 0) {
                        status = Core::ERROR_NONE;
                        break;
                    }
                }
                if (status != Core::ERROR_NONE) {
                    string name;
                    if (weston_compositor_find_output_by_name(_parent->PlatformCompositor(), layoutput->Name.c_str()) == 0) {
                        name = layoutput->Name;
                    }
                    while (layoutput->Clones.size()) {
                        if (!wl_list_empty(&layoutput->OutputList)) {
                            weston_log("Error: independent-CRTC clone mode is not implemented.\n");
                            break;
                        }

                        if (name.empty() == true) {
                            name = layoutput->Name + ":" + string(weston_head_get_name(layoutput->Clones.front()));
                        }

                        output = LayoutputCreateOutput(layoutput, name);

                        if (!output) {
                            break;
                        }

                        if (TryAttachEnable(output->Output, layoutput) != Core::ERROR_NONE) {
                            DestroyOutput(output);
                            break;
                        }

                        status = Core::ERROR_NONE;
                    }
                }
                return status;
            }
            uint32_t ProcessLayoutputs()
            {
                IBackend::Layoutput* layoutput;
                uint32_t status = Core::ERROR_NONE;

                wl_list_for_each(layoutput, &_layoutputList, CompositorLink) {
                    if (layoutput->Clones.size()) {

                        if (ProcessLayoutput(layoutput) != Core::ERROR_NONE) {
                            layoutput->Clones.clear();
                            status = Core::ERROR_GENERAL;
                        }
                    }
                }

                return status;
            }
            void HeadDisable(struct weston_head* head)
            {
                HeadTracker* track = GetHeadTracker(head);
                if (track) {
                    DestroyHeadTracker(track);
                }

                struct weston_output* outputBase = weston_head_get_output(head);
                struct OutputData* output = GetOutputFromWeston(outputBase);

                weston_head_detach(head);
                if (CountRemainingHeads(output->Output, nullptr) == 0) {
                    DestroyOutput(output);
                }
            }
            static void HeadsChanged(struct wl_listener* listener, void* argument)
            {
                TRACE_GLOBAL(Trace::Information, (_T("DRM Backend Loading in progress")));
                struct weston_compositor* compositor = static_cast<weston_compositor*>(argument);
                Compositor::UserData* userData = static_cast<Compositor::UserData*>(weston_compositor_get_user_data(compositor));
                Compositor* parent = userData->Parent;

                struct weston_head* head = nullptr;

                while ((head = weston_compositor_iterate_heads(compositor, head))) {
                    bool connected = weston_head_is_connected(head);
                    bool enabled = weston_head_is_enabled(head);
                    bool changed = weston_head_is_device_changed(head);
                    bool forced = parent->Backend()->Forced();
                    if ((connected || forced) && !enabled) {
                        static_cast<DRM*>(parent->Backend())->HeadPrepareEnable(head);
                    } else if (!(connected || forced) && enabled) {
                        static_cast<DRM*>(parent->Backend())->HeadDisable(head);
                    } else if (enabled && changed) {
                        weston_log("Detected a monitor change on head '%s', "
                                   "not bothering to do anything about it.\n",
                                   weston_head_get_name(head));
                    }
                    weston_head_reset_device_changed(head);
                }

                if (static_cast<DRM*>(parent->Backend())->ProcessLayoutputs() == Core::ERROR_NONE) {
                    parent->Loaded(true);
                }
            }
            uint32_t UpdateResolution(const string& mode) override {
                uint32_t status = Core::ERROR_UNAVAILABLE;
                struct weston_head* head = nullptr;
                while ((head = weston_compositor_iterate_heads(_parent->PlatformCompositor(), head))) {
                    bool connected = weston_head_is_connected(head);
                    if (connected) {
                        struct weston_output *output = weston_head_get_output(head);
                        string name = weston_head_get_name(head);
                        Output config = OutputConfig(name);
                        if (config.Name().empty() != true) {
                            Output newConfig = Output(config.Name(), mode, config.Transform());

                            weston_output_disable(output);
                            output->scale = 0;
                            ConfigureOutput(output, newConfig);
                            weston_output_enable(output);
                            status = Core::ERROR_NONE;
                        }
                    }
                }
                return status;
            }

        private:
            std::list<uint8_t> _tty;
            struct wl_listener _listener;
            Compositor* _parent;
        };

    public:
        struct UserData {
            struct weston_compositor *WestonCompositor;
            struct weston_config *Config;
            Compositor* Parent;
        };
    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Shell(_T("/usr/lib/weston/desktop-shell.so"))
                , ConfigLocation(_T(""))
            {
                Add(_T("shellfilelocation"), &Shell);
                Add(_T("configlocation"), &ConfigLocation);
            }
            ~Config() = default;

        public:
            Core::JSON::String Shell;
            Core::JSON::String ConfigLocation;
        };

    private:
        static constexpr const uint8_t MaxCloneHeads = 16;
        static constexpr const uint32_t MaxLoadingTime = 3000;
        static constexpr const TCHAR* ShellInitEntryAPI = _T("wet_shell_init");
        static constexpr const TCHAR* ShellSurfaceUpdateLayerAPI = _T("wet_shell_surface_update_layer");

    public:
        Compositor() = delete;
        Compositor(const Compositor&) = delete;
        Compositor& operator= (const Compositor&) = delete;

        Compositor(PluginHost::IShell* service)
            : _surfaceSetTopAPI(nullptr)
            , _logger(service)
            , _userData()
            , _backend(nullptr)
            , _inputController(this)
            , _displayController(nullptr)
            , _callback(nullptr)
            , _backendLoaded(false)
            , _service(nullptr)
            , _display(nullptr)
            , _compositor(nullptr)
            , _resolution(Exchange::IComposition::ScreenResolution_1080i50Hz)
            , _loadedSignal(false, true)
            , _adminLock()
        {
            TRACE(Trace::Information, (_T("Starting Compositor")));

            ASSERT(_instance == nullptr);
            _instance = this;
            _service = service;

            string runtimeDir;
            Core::SystemInfo::GetEnvironment(_T("XDG_RUNTIME_DIR"), runtimeDir);

            Core::File path(runtimeDir);

            if (path.IsDirectory() == false) {
                Core::Directory(path.Name().c_str()).CreatePath();
                TRACE(Trace::Information, (_T("Created XDG_RUNTIME_DIR: %s\n"), path.Name().c_str()));
            }

            // Start compositor loading
            Run();

            // Wait till it get loaded successfully
            bool compositorLoaded = WaitForLoading();

            ASSERT(compositorLoaded == true);
            if (compositorLoaded == false) {
                TRACE(Trace::Error, (_T("Failed to load Weston Compositor")));
            }
        }
        ~Compositor() {
            TRACE(Trace::Information, (_T("Destructing the compositor")));
            delete _backend;
            _logger.DestroyLogScope();

            ClearSignal();
            weston_compositor_tear_down(_compositor);
            weston_log_ctx_compositor_destroy(_compositor);
            weston_compositor_destroy(_compositor);
            wl_display_terminate(_compositor->wl_display); //FIXME: revisit exti sequence
            wl_display_destroy(_display);

            Stop();
            Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
            _instance = nullptr;
            _service = nullptr;
        }

        uint32_t StartComposition() {
            uint32_t status = Core::ERROR_GENERAL;
            _display = wl_display_create();

            ASSERT(_display != nullptr);
            if (_display != nullptr) {

                _userData.Config = nullptr;
                _userData.Parent = this;
                _compositor = weston_compositor_create(_display, _logger.Context(), &_userData);
                if (_compositor != nullptr ) {
                    _userData.WestonCompositor = _compositor;
                    struct xkb_rule_names xkb_names = {0};
                    int err = weston_compositor_set_xkb_rule_names(_compositor, &xkb_names);
                    ASSERT(err == 0);
                    if (err == 0) {
                        weston_init_process_list();
                        if (LoadBackend(_service) == true) {
                            _compositor->idle_time = 300;
                            _compositor->default_pointer_grab = NULL;
                            _compositor->exit = HandleExit;

                            const char* socketName = wl_display_add_socket_auto(_display);
                            ASSERT(socketName);
                            if (socketName) {
                                Config config;
                                config.FromString(_service->ConfigLine());
                                SetEnvironments(config, socketName);
                                if (LoadShell(config.Shell.Value()) == Core::ERROR_NONE) {
                                    RegisterSurfaceActivateListener();
                                    _inputController.CollectInputHandlers();
                                    weston_compositor_wake(_compositor);
                                    NotifyLoaded();
                                    wl_display_run(_display);
                                    status = Core::ERROR_NONE;
                                }
                            }
                        }
                    }
                }
            }
            return status;
        }
        inline bool LoadBackend(PluginHost::IShell* service)
        {
            _backend = Create<DRM>(_service);
            _backend->Load(this);
            weston_compositor_flush_heads_changed(_compositor);
            ASSERT(_backendLoaded == true);
            return (_backendLoaded);
        }
        inline void SetEnvironments(const Config& config, const string& socketName)
        {
            string configLocation = (config.ConfigLocation.IsSet() == true) ?
                   config.ConfigLocation.Value() : _service->DataPath();
            Core::SystemInfo::SetEnvironment(_T("XDG_CONFIG_DIRS"), configLocation);
            Core::SystemInfo::SetEnvironment(_T("WAYLAND_DISPLAY"), socketName);
        }
        template <class BACKEND>
        IBackend* Create(PluginHost::IShell* service)
        {
            IBackend* backend = new BACKEND(service);
            return backend;
        }
        static Compositor* Create(PluginHost::IShell* service)
        {
            return _instance == nullptr ? new Weston::Compositor(service) : _instance;
        }
        static Compositor* Instance()
        {
            return (_instance);
        }

    public:
        inline struct weston_compositor* PlatformCompositor()
        {
            return _compositor;
        }
        inline IBackend* Backend()
        {
            return _backend;
        }
        inline void Loaded(const bool success)
        {
            _backendLoaded = success;
        }
        inline void NotifyLoaded()
        {
            _loadedSignal.SetEvent();
        }
        inline void ClearSignal()
        {
            _loadedSignal.ResetEvent();
        }
        inline bool WaitForLoading() {
            return (_loadedSignal.Lock(MaxLoadingTime) == Core::ERROR_NONE ? true : false);
        }
        void Get(const uint32_t id, Wayland::Display::Surface& surface) override
        {
            _adminLock.Lock();
            for (const auto& index: _surfaces) {
                if (index.second->Id() == id) {
                    surface = Wayland::Display::Surface(index.second);
                    break;
                }
            }
            _adminLock.Unlock();
        }
        void Process(Wayland::Display::IProcess* processloop) override
        {
            _displayController->Process(processloop);
        }
        void SetInput(const string& name) override
        {
            struct weston_surface* surface = GetSurface(name);
            if (surface) {
                _inputController.SetInput(surface);
            }
        }
        bool StartController(const string& name, Wayland::Display::ICallback *callback) override
        {
            bool status = false;
            _displayController = &(Wayland::Display::Instance(name));
            if (_displayController != nullptr) {
                // OK ready to receive new connecting surfaces.
                _callback = callback;

                // We also want to be know the current surfaces.
                _displayController->LoadSurfaces();
                status = true;
            }
            return status;
        }
        uint32_t SetResolution(const Exchange::IComposition::ScreenResolution value) {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            const char* request = nullptr;
            switch(value) {
                case Exchange::IComposition::ScreenResolution_480i:      request = "720x480@60.0";       break;
                case Exchange::IComposition::ScreenResolution_480p:      request = "720x480@60.0 16:9";  break;
                case Exchange::IComposition::ScreenResolution_720p:      request = "1280x720@60.0 16:9"; break;
                case Exchange::IComposition::ScreenResolution_720p50Hz:  request = "1280x720@50.0";      break;
                case Exchange::IComposition::ScreenResolution_1080p24Hz: request = "1920x1080@24.0";     break;
                case Exchange::IComposition::ScreenResolution_1080i50Hz: request = "1920x1080@50.0";     break;
                case Exchange::IComposition::ScreenResolution_1080p50Hz: request = "1920x1080@50.0";     break;
                case Exchange::IComposition::ScreenResolution_1080p60Hz: request = "1920x1080@60.0";     break;
                case Exchange::IComposition::ScreenResolution_2160p50Hz: request = "3840x2160@50.0";     break;
                case Exchange::IComposition::ScreenResolution_2160p60Hz: request = "3840x2160@60.0";     break;
                default: break;
            }
            if (request != nullptr) {
                 result = _backend->UpdateResolution(request);
                _resolution = value;
            } else {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            return (result);
        }
        Exchange::IComposition::ScreenResolution GetResolution() const {
            return (_resolution);
        }

    private:
        uint32_t Worker() override
        {
            if (IsRunning() == true) {
                StartComposition();
            }
            Block();
            return (Core::infinite);
        }
        inline void RegisterSurfaceActivateListener() {
            _surfaceActivateListener.notify = NotifySurfaceActivate;
            wl_signal_add(&_compositor->activate_signal, &_surfaceActivateListener);
        }
        static void NotifySurfaceActivate(struct wl_listener* listener, void* data)
        {
            struct weston_surface_activation_data* activationData = static_cast<struct weston_surface_activation_data*>(data);
            struct weston_surface* surface = activationData->surface;
            char label[100];
            if (surface->get_label) {
                int status =  surface->get_label(surface, label, sizeof(label));
                if (status >= 0) {
                    Compositor::UserData* userData =
                            static_cast<Compositor::UserData*>(weston_compositor_get_user_data(surface->compositor));
                    userData->Parent->AddSurface(label, surface);
                }
            }
        }
        static void HandleExit(struct weston_compositor* compositor)
        {
            wl_display_terminate(compositor->wl_display);
        }
        uint32_t LoadShell(const string& shell)
        {
            uint32_t status = Core::ERROR_GENERAL;
            typedef int (*ShellInit)(struct weston_compositor*, int*, char*[]);

            Core::Library library(shell.c_str());
            _library = library;
            if (library.IsLoaded() == true) {
                ShellInit shellInit = reinterpret_cast<ShellInit>(library.LoadFunction(ShellInitEntryAPI));
                if (shellInit != nullptr) {
                    if (shellInit(_compositor, 0, NULL) == 0) {
                        _surfaceSetTopAPI = reinterpret_cast<ShellSurfaceSetTop>(library.LoadFunction(ShellSurfaceUpdateLayerAPI));
                        status = Core::ERROR_NONE;
                    }
                }
            }
            ASSERT(status == Core::ERROR_NONE)
            return status;
        }
        string ClientName(const string& label)
        {
            size_t first = label.find_first_of("\'");
            size_t last = label.find_last_of("\'");

            string clientName = label.substr(first + 1, last - (first + 1));
            return clientName;
        }
        void AddSurface(const string& label, struct weston_surface* westonSurface)
        {
            string name = ClientName(label);
            _adminLock.Lock();
            SurfaceMap::iterator index(_surfaces.find(name));
            if (index == _surfaces.end()) {
                uint16_t id = random();
                SurfaceData* surface = new SurfaceData(this, id, name, westonSurface);
                _surfaces.insert(std::pair<const string, SurfaceData*>(name, surface));
                surface->AddRef();
                _callback->Attached(id);
                if (name == "video-surface") {
                    SurfaceMap::iterator index = _surfaces.begin();
                    if (index != _surfaces.end()) {
                        _callback->Detached(index->second->Id());
                        _callback->Attached(index->second->Id());
                        SetInput(index->second->Name());
                    }
                }

            } else {
                _surfaces.erase(index);
                _surfaces.insert(std::pair<const string, SurfaceData*>(name, index->second));
            }
            _adminLock.Unlock();
        }
        void RemoveSurface(struct weston_surface* westonSurface)
        {
            _adminLock.Lock();
            for (const auto& index: _surfaces) {
                if (index.second->WestonSurface() == westonSurface) {
                    _callback->Detached(index.second->Id());
                    _surfaces.erase(index.first);
                    index.second->Release();
                    break;
                }
            }
            _adminLock.Unlock();
        }
        struct weston_surface* GetSurface(const string& name)
        {
            struct weston_surface* surface = nullptr;
            _adminLock.Lock();
            const auto& index(_surfaces.find(name));
            if (index != _surfaces.end()) {
                surface = index->second->WestonSurface();
            }
            _adminLock.Unlock();

            return surface;
        }
    public:
        ShellSurfaceSetTop _surfaceSetTopAPI;

    private:
        Logger _logger;
        UserData _userData;
        IBackend* _backend;
        SurfaceMap _surfaces;
        InputController _inputController;
        Wayland::Display* _displayController;
        Wayland::Display::ICallback* _callback;

        bool _backendLoaded;
        Core::Library _library;
        PluginHost::IShell* _service;

        struct wl_display* _display;
        struct weston_compositor* _compositor;
        struct wl_listener _surfaceActivateListener;
        Exchange::IComposition::ScreenResolution _resolution;

        mutable Core::Event _loadedSignal;
        mutable Core::CriticalSection _adminLock;

        static Weston::Compositor* _instance;
    };

    /*static*/ Core::File Weston::Compositor::Logger::_logFile;
    /*static*/ Weston::Compositor* Weston::Compositor::_instance = nullptr;
    /*static*/ struct weston_log_scope* Weston::Compositor::Logger::_logScope = nullptr;
    /*static*/ Weston::Compositor::InputController* Weston::Compositor::InputController::_instance = nullptr;

} // namespace Weston

    Exchange::IComposition::ScreenResolution GetResolution() {
        return (Weston::Compositor::Instance()->GetResolution());
    }
    uint32_t SetResolution(Exchange::IComposition::ScreenResolution value) {
        return (Weston::Compositor::Instance()->SetResolution(value));
    }
    IServer* Create(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        Weston::Compositor* instance = Weston::Compositor::Create(service);

        return instance;
    }

} // namespace Implementation
} // namespace WPEFramework
