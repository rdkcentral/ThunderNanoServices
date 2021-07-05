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
#include <weston/shared/helpers.h>

namespace WPEFramework {
namespace Implementation {
namespace Weston {

    class Compositor : public Implementation::IServer, Core::Thread {
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
            static int Log(const char* format, va_list argp)
            {
                int length = 0;

                if (weston_log_scope_is_enabled(_logScope)) {
                    char* str;
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


    private:
        class IBackend {
        private:
            class Connector : public Core::JSON::Container {
            private:
                Connector& operator=(const Connector&) = delete;

            public:
                Connector()
                    : Core::JSON::Container()
                    , Name()
                    , Mode()
                    , Transform()
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
                    Add(_T("forces"), &Forced);
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
            ~IBackend() {
                  _outputs.clear();
            }

        protected:
            void OutputDestroy(OutputData* output)
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
        protected:
            bool _forced;
            std::map<const string, Output> _outputs;
            struct wl_list _layoutputList;
        };

        class DRM : public IBackend{
        private:
            static constexpr const TCHAR* OutputModePreferred = _T("preferred");
        private:
            class Config : public Core::JSON::Container {
            private:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

            public:
                Config()
                    : Core::JSON::Container()
                    , TTY(1)
                {
                    Add(_T("tty"), &TTY);
                }
                ~Config()
                {
                }

            public:
                Core::JSON::DecUInt8 TTY;
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
                _tty = config.TTY.Value();

                wl_list_init(&_layoutputList);
            }

        public:
            void Load(Compositor* parent) override
            {
                TRACE(Trace::Information, (_T("Loading DRM Backend")));
                struct weston_drm_backend_config config = {{ 0, }};

                config.tty = _tty;
                config.base.struct_version = WESTON_DRM_BACKEND_CONFIG_VERSION;
                config.base.struct_size = sizeof(struct weston_drm_backend_config);

                _listener.notify = HeadsChanged;
                _parent = parent;
                struct weston_compositor* compositor = parent->PlatformCompositor();
                weston_compositor_add_heads_changed_listener(compositor, &_listener);

                weston_compositor_load_backend(compositor, WESTON_BACKEND_DRM, &config.base);
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
                Layoutput* layoutput;

                wl_list_for_each(layoutput, &_layoutputList, CompositorLink) {
                    if (layoutput->Name == name) {
                        break;
                    }
                }
                return layoutput;
            }
            Layoutput* CreateLayoutput(const string& name, const IBackend::Output& output)
            {
                Layoutput* layoutput = new Layoutput;
                ASSERT(layoutput);
                if (layoutput) {
                    wl_list_insert(_layoutputList.prev, &layoutput->CompositorLink);
                    wl_list_init(&layoutput->OutputList);
                    layoutput->Name = name;
                    layoutput->Config = output;
                }
               return layoutput;
            }
            void LayoutputAddHead(struct weston_head* head, const IBackend::Output& output)
            {
                const char* outputName = weston_head_get_name(head);
                Layoutput* layoutput = FindLayoutput(outputName);
                if (layoutput == nullptr) {
                    CreateLayoutput(outputName, output);
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
            uint32_t OutputConfigure(struct weston_output* output)
            {
                uint32_t status = Core::ERROR_NONE;
                const struct weston_drm_output_api* api = weston_drm_output_get_api(_parent->PlatformCompositor());
                if (api) {
                    if (!api->set_mode(output, WESTON_DRM_BACKEND_OUTPUT_PREFERRED, OutputModePreferred)) {
                        api->set_gbm_format(output, NULL);
                        api->set_seat(output, "");
                        weston_output_set_scale(output, 1);
                        weston_output_set_transform(output, WL_OUTPUT_TRANSFORM_NORMAL);
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
                for (std::list<struct weston_head*>::iterator clone; clone != Clones.begin(); clone++) {
                    if (weston_output_attach_head(output,* clone) < 0) {

                        failedClones.push_back(*clone);
                        Clones.erase(clone);
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
                status = OutputConfigure(output);
                if (status != Core::ERROR_NONE) {

                    status = TryEnable(output, layoutput->Clones, failedClones);
                    if (status != Core::ERROR_NONE) {

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
                        OutputDestroy(output);
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

                        if (TryAttachEnable(output->Output, layoutput) < 0) {
                            OutputDestroy(output);
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
                    OutputDestroy(output);
                }
            }
            static void HeadsChanged(struct wl_listener* listener, void* argument)
            {
                TRACE_GLOBAL(Trace::Information, (_T("Loading DRM Backend")));
                struct weston_compositor* compositor = static_cast<weston_compositor*>(argument);
                Compositor::WestonUserData* userData = static_cast<Compositor::WestonUserData*>(weston_compositor_get_user_data(compositor));
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

                if (static_cast<DRM*>(parent->Backend())->ProcessLayoutputs() != Core::ERROR_NONE) {
                    parent->Initialized(true);
                }
            }
        private:
            uint8_t _tty;
            struct wl_listener _listener;
            Compositor* _parent;
        };

    public:
        struct WestonUserData {
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
                , ShellFileLocation(_T("/usr/lib/weston"))
            {
                Add(_T("shellfilelocation"), &ShellFileLocation);
            }
            ~Config() = default;

        public:
            Core::JSON::String ShellFileLocation;
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

                _userData.Config = nullptr;
                _userData.Parent = this;
                _compositor = weston_compositor_create(_display, _logger.Context(), &_userData);
                if (_compositor != nullptr ) {
                    _userData.WestonCompositor = _compositor;
                    struct xkb_rule_names xkb_names = {0};
                    int err = weston_compositor_set_xkb_rule_names(_compositor, &xkb_names);
                    ASSERT(err == 0);
                    if (err == 0) {
                        _backend = Create<DRM>(service);
                        _backend->Load(this);
                        weston_compositor_flush_heads_changed(_compositor);
                        ASSERT(_initialized == true);

                        _compositor->idle_time = 300;
                        _compositor->default_pointer_grab = NULL;
                        _compositor->exit = HandleExit;

                        const char* socketName = wl_display_add_socket_auto(_display);
                        ASSERT(socketName);
                        if (socketName) {
                            Core::SystemInfo::SetEnvironment(_T("WAYLAND_DISPLAY"), socketName);
                            Config config;
                            config.FromString(service->ConfigLine());

                            uint32_t status = LoadShell(config.ShellFileLocation.Value());
                            ASSERT(status == Core::ERROR_NONE)
                            if (status == Core::ERROR_NONE) {
                                weston_compositor_wake(_compositor);
                                Run();
                            }
                        }
                    }
                }
            }
        }
        ~Compositor() {
            TRACE(Trace::Information, (_T("Destructing the compositor")));
            Block();
            weston_compositor_tear_down(_compositor);
            weston_log_ctx_compositor_destroy(_compositor);
            weston_compositor_destroy(_compositor);
            wl_display_destroy(_display);
            Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
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
        inline void Initialized(const bool success)
        {
            _initialized = success;
        }
        void SetInput(const char name[]) override
        {
        }
    private:
        uint32_t Worker() override
        {
            while (IsRunning() == true) {
                wl_display_run(_display);
            }
            return (Core::infinite);
        }
        static void HandleExit(struct weston_compositor* compositor)
        {
            wl_display_terminate(compositor->wl_display);
        }
        uint32_t LoadShell(const string shellFileLocation)
        {
            uint32_t status = Core::ERROR_GENERAL;
            typedef int (*ShellInit)(struct weston_compositor*, int*, char*[]);

            string shellFileName = string(Core::Directory::Normalize(shellFileLocation) + DesktopShell);
            Core::Library library(shellFileName.c_str());
            void* handle = dlopen(shellFileName.c_str(), RTLD_LAZY);
            if (library.IsLoaded() == true) {
                ShellInit shellInit = reinterpret_cast<ShellInit>(library.LoadFunction(ShellInitEntryName));
                if (shellInit != nullptr) {
                    if (shellInit(_compositor, 0, NULL) == 0) {
                        status = Core::ERROR_NONE;
                    }
                }
            }
            return status;
        }

    private:
        Logger _logger;
        IBackend* _backend;

        bool _initialized;
        struct wl_display* _display;
        WestonUserData _userData;
        struct weston_compositor* _compositor;

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
