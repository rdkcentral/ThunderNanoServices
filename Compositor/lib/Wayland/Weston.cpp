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

WL_EXPORT void* wet_load_module_entrypoint(const char*, const char*);

#include <libweston/libweston.h>
#include <libweston/weston-log.h>
#include <libweston/backend-drm.h>

namespace WPEFramework {
namespace Implementation {
namespace Weston {

    class Compositor : public Implementation::IServer {

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
                ~Config()
                {
                }

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
                    (config.LogFilePath.IsSet() == true) ? config.LogFilePath.Value() : service->VolatilePath();
                _logFile = Core::File(Core::Directory::Normalize(logFilePath) + config.LogFileName.Value());

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
                weston_compositor_log_scope_destroy(_logScope);
                _logScope = nullptr;
                UnSubscribeLogger();
                _logFile.Close();
            }
            struct weston_log_context* Context() const
            {
                return _context;
            }

        private:
            inline void SetHandler()
            {
                if (_logFile.Create() == true) {
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
            static int LogContinue(const char* format, va_list argp)
            {
                return weston_log_scope_vprintf(_logScope, format, argp);
            }
            static int Log(const char *format, va_list argp)
            {
                int length = 0;

                if (weston_log_scope_is_enabled(_logScope)) {
                    char *str;
                    Core::Time now(Core::Time::Now());
                    string timestamp(now.ToRFC1123(true));
                    length = vasprintf(&str, format, argp);
                    if (length >= 0) {
                        length = weston_log_scope_printf(_logScope, "%s %s", timestamp.c_str(), str);
                        free(str);
                    } else {
                        length = weston_log_scope_printf(_logScope, "%s Out of memory", timestamp.c_str());
                    }
                }
                return length;
            }
        private:
            struct weston_log_context* _context;
            struct weston_log_subscriber* _logger;
            struct weston_log_subscriber* _flightRec;
            static struct weston_log_scope* _logScope;

            Core::File _logFile;
        };

        class IBackend {
        public:
            IBackend(const IBackend&) = delete;
            IBackend& operator=(const IBackend&)= delete;
            IBackend() = default;
            ~IBackend() = default;
        public:
            virtual bool Forced() const = 0;
            virtual void Load(struct weston_compositor* compositor) = 0;
        };
        class DRM : public IBackend{
        private:
            class Config : public Core::JSON::Container {
            private:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

            public:
                Config()
                    : Core::JSON::Container()
                    , TTY(1)
                    , Forced(false)
                {
                    Add(_T("tty"), &TTY);
                    Add(_T("forces"), &Forced);
                }
                ~Config()
                {
                }

            public:
                Core::JSON::DecUInt8 TTY;
                Core::JSON::Boolean Forced;
            };

        public:
            DRM() = delete;
            DRM(const DRM&) = delete;
            DRM& operator=(const DRM&)= delete;
            DRM(PluginHost::IShell* service)
                : _tty()
                , _forced(false)
                , _listener()
            {
                Config config;
                config.FromString(service->ConfigLine());
                _tty = config.TTY.Value();
                _forced = config.Forced.Value();
            }

        public:
            inline bool Forced() const override
            {
                return _forced;
            }
            void Load(struct weston_compositor* compositor) override
            {
                struct weston_drm_backend_config config = {{ 0, }};

                config.tty = _tty;
                config.base.struct_version = WESTON_DRM_BACKEND_CONFIG_VERSION;
                config.base.struct_size = sizeof(struct weston_drm_backend_config);

                _listener.notify = DRMHeadsChanged;
                weston_compositor_add_heads_changed_listener(compositor, &_listener);

                weston_compositor_load_backend(compositor, WESTON_BACKEND_DRM, &config.base);
            }
        private:
            static void DRMHeadsChanged(struct wl_listener* listener, void* argument)
            {
                 struct weston_compositor* compositor = static_cast<weston_compositor*>(argument);
                 Compositor* parent = static_cast<Compositor*>(weston_compositor_get_user_data(compositor));
                 struct weston_head* head = nullptr;

                 while ((head = weston_compositor_iterate_heads(compositor, head))) {
                     bool connected = weston_head_is_connected(head);
                     bool enabled = weston_head_is_enabled(head);
                     bool changed = weston_head_is_device_changed(head);
                     bool forced = parent->Backend()->Forced();

                     if ((connected || forced) && !enabled) {
                        //drm_head_prepare_enable(wet, head);
                        //LayOutputAddHead(parent, head);
                     } else if (!(connected || forced) && enabled) {
                        //drm_head_disable(head);
                     } else if (enabled && changed) {
                        weston_log("Detected a monitor change on head '%s', "
                                   "not bothering to do anything about it.\n",
                                   weston_head_get_name(head));
                     }
                     weston_head_reset_device_changed(head);
                }

                /*if (DrmProcessLayOutputs(parent) >= 0) {
                    parent->Initialized(true);
                }*/
            }
        private:
            uint8_t _tty;
            bool _forced;
            struct wl_listener _listener;
        };

    public:
        struct LayOutput {
            string Name;
            struct wl_list CompositorLink;
            struct wl_list OutputList;
            std::list<struct weston_head*> Clones;
        };
    private:
        static constexpr const uint8_t MaxCloneHeads = 16;
        static constexpr const TCHAR* DesktopShell = _T("desktop-shell.so");
        static constexpr const TCHAR* ShellInitEntryName = _T("wet_shell_init");

    public:
        Compositor() = delete;
        Compositor(const Compositor&) = delete;
        Compositor& operator= (const Compositor&) = delete;

        Compositor(PluginHost::IShell* service)
            : _logger(service)
            , _initialized(false)
            , _display(nullptr)
            , _compositor(nullptr)
        {
            TRACE(Trace::Information, (_T("Starting Compositor")));

            ASSERT(_instance == nullptr);
            _instance = this;

            string runtimeDir;
            Core::SystemInfo::GetEnvironment(_T("XDG_RUNTIME_DIR"), runtimeDir);

            Core::File path(runtimeDir);

            if (path.IsDirectory() == false) {
                Core::Directory(path.Name().c_str()).CreatePath();
                TRACE(Trace::Information, (_T("Created XDG_RUNTIME_DIR: %s\n"), path.Name().c_str()));
            }

            _display = wl_display_create();

            ASSERT(_display != nullptr);
            if (_display != nullptr) {

                _compositor = weston_compositor_create(_display, _logger.Context(), this);
                if (_compositor != nullptr ) {
                    struct xkb_rule_names xkb_names = {0};
                    int err = weston_compositor_set_xkb_rule_names(_compositor, &xkb_names);
                    ASSERT(err == 0);
                    if (err != 0) {
                        _backend = Create<DRM>(service);
                        _backend->Load(_compositor);
                        weston_compositor_flush_heads_changed(_compositor);
                        ASSERT(_initialized == true);

                        _compositor->idle_time = 300;
                        _compositor->default_pointer_grab = NULL;
                        _compositor->exit = HandleExit;

                        const char *socketName = wl_display_add_socket_auto(_display);
                        ASSERT(socketName);
                        if (socketName) {
                            Core::SystemInfo::SetEnvironment(_T("WAYLAND_DISPLAY"), socketName);
                            uint32_t status = LoadShell();
                            ASSERT(status == Core::ERROR_NONE)
                            if (status == Core::ERROR_NONE) {
                                weston_compositor_wake(_compositor);
                            }
                        }
                    }
                }
            }
        }
        ~Compositor() {
            TRACE(Trace::Information, (_T("Destructing the compositor")));
            weston_compositor_tear_down(_compositor);
            weston_log_ctx_compositor_destroy(_compositor);
            weston_compositor_destroy(_compositor);
            wl_display_destroy(_display);
            _instance = nullptr;
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
        static Compositor* Instance() {
            return (_instance);
        }

    public:
        inline IBackend* Backend() {
            return _backend;
        }
        inline struct wl_list& LayOutputList() {
            return _layOutputList;
        }
        inline void Initialized(const bool success) {
            _initialized = success;
        }
        void SetInput(const char name[]) override
        {
        }

        static LayOutput* FindLayoutput(Compositor* compositor, const string& name)
        {
            LayOutput* layOutput;

            wl_list_for_each(layOutput, &compositor->LayOutputList(), CompositorLink) {
                if (layOutput->Name == name) {
                    break;
                }
            }
            return layOutput;
        }
        static void LayOutputAddHead(Compositor* compositor, struct weston_head* head)
        {
            const char *outputName = weston_head_get_name(head);
            LayOutput* layOutput = FindLayoutput(compositor, outputName);
            if (layOutput == nullptr) {
                // Create LayOutput
            }
            if (layOutput->Clones.size() < MaxCloneHeads) {
                layOutput->Clones.push_back(head);
            }
            return;
        }
    private:
        static void HandleExit(struct weston_compositor* compositor)
        {
            wl_display_terminate(compositor->wl_display);
        }
        uint32_t LoadShell()
        {
            uint32_t status = Core::ERROR_NONE;
            typedef int (*ShellInit)(struct weston_compositor*, int*, char*[]);

            ShellInit shellInit = reinterpret_cast<ShellInit>(wet_load_module_entrypoint(DesktopShell, ShellInitEntryName));
            if (shellInit) {
               if (shellInit(_compositor, 0, NULL) < 0) {
                   status = Core::ERROR_GENERAL;
               }
            } else {
               status = Core::ERROR_UNAVAILABLE;
            }
            return status;
        }

    private:
        Logger _logger;
        IBackend* _backend;

        bool _initialized;
        struct wl_display* _display;
        struct weston_compositor* _compositor;

        struct wl_list _layOutputList;
        static Weston::Compositor* _instance;
    };

    /*static*/ Weston::Compositor* Weston::Compositor::_instance = nullptr;
    /*static*/ struct weston_log_scope* Weston::Compositor::Logger::_logScope = nullptr;

} // namespace Weston

    IServer* Create(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        Weston::Compositor* instance = Weston::Compositor::Create(service);

        return instance;
    }

} // namespace Implementation
} // namespace WPEFramework
