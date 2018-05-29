#include <nexus_config.h>
#include <nexus_types.h>
#include <nexus_platform.h>
#include <nxclient.h>
#include <nxserverlib.h>
#include <nexus_display_vbi.h>
#include <nexus_otpmsp.h>
#include <nexus_read_otp_id.h>

#include <stdio.h>
#include <string.h>

/* print_capabilities */
#if NEXUS_HAS_VIDEO_DECODER
#include <nexus_video_decoder.h>
#endif
#include <nexus_display.h>

BDBG_MODULE(NexusWrapper);

#include "Module.h"

#include <interfaces/IResourceCenter.h>
#include <interfaces/IComposition.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {

    /* -------------------------------------------------------------------------------------------------------------
     * This is a singleton. Declare all C accessors to this object here
     * ------------------------------------------------------------------------------------------------------------- */
    class PlatformImplementation;

    static PlatformImplementation* _implementation = nullptr;
    static Core::CriticalSection _implementationLock;

    /* -------------------------------------------------------------------------------------------------------------
     *
     * ------------------------------------------------------------------------------------------------------------- */
    class PlatformImplementation : public Exchange::IResourceCenter,
                                   public Exchange::IComposition {
    private:
        PlatformImplementation(const PlatformImplementation&) = delete;
        PlatformImplementation& operator=(const PlatformImplementation&) = delete;

        enum ServerMode {
            UNINITIALIZED,
            INTERNAL_SERVER_RUNNING,
            EXTERNAL_SERVER_JOINED
        };
        /* -------------------------------------------------------------------------------------------------------------
         *  NX server lib stuff
         * ------------------------------------------------------------------------------------------------------------- */
        class Entry : public Exchange::IComposition::IClient {
        private:
            Entry() = delete;
            Entry(const Entry&) = delete;
            Entry& operator=(const Entry&) = delete;

        protected:
            Entry(nxclient_t client, const NxClient_JoinSettings* settings)
                : _client(client)
                , _settings(*settings)
            {
                TRACE_L1("Created client named: %s", _settings.name);
            }

        public:
            static Entry* Create(nxclient_t client, const NxClient_JoinSettings& settings)
            {
                Entry* result = Core::Service<Entry>::Create<Entry>(client, &settings);

                return (result);
            }
            virtual ~Entry()
            {
                TRACE_L1("Destructing client named: %s", _settings.name);
            }

        public:
            inline bool IsActive() const
            {
                return (_client != nullptr);
            }
            inline const char* Id() const
            {
                ASSERT(_client != nullptr);

                return (_settings.name);
            }
            virtual string Name() const override
            {
                return (string(Id()));
            }
            virtual void Kill() override
            {
                ASSERT(_client != nullptr);

                /* must call nxserver_ipc so it can unwind first. */
                nxserver_ipc_close_client(_client);

                TRACE(Trace::Information, (_T("Kill client %s."), Name().c_str()));

                /* We expect the Disconnect Client to be triggered by the previous call
                   TODO: Double check this is true, and if not call it explicitly here. */
            }
            virtual void Opacity(const uint32_t value) override
            {
                ASSERT(_client != nullptr);

                TRACE(Trace::Information, (_T("Alphs client %s to %d."), Name().c_str(), value));
                nxserverlib_set_server_alpha(_client, value);
            }

            virtual void Geometry(const uint32_t X, const uint32_t Y, const uint32_t width, const uint32_t height) override
            {
                // TODO
            }

            virtual void Visible(const bool visible) override
            {
                // TODO
            }

            virtual void SetTop()
            {
                ASSERT(_client != nullptr);

                /* the definition of "focus" is variable. this is one impl. */
                NxClient_ConnectList list;
                struct nxclient_status status;

                nxclient_get_status(_client, &status);
                NxClient_P_Config_GetConnectList(_client, status.handle, &list);

                /* only refresh first connect */
                if (list.connectId[0]) {
                    NxClient_P_RefreshConnect(_client, list.connectId[0]);
                }

                TRACE(Trace::Information, (_T("Focus client %s."), Name().c_str()));

                nxserver_p_focus_surface_client(_client);
            }

            virtual void SetInput()
            {
                // TODO
            }

            BEGIN_INTERFACE_MAP(Entry)
            INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

        private:
            nxclient_t _client;
            NxClient_JoinSettings _settings;
        };

        class Job : public Core::Thread {
        private:
            Job() = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

        public:
            Job(PlatformImplementation& parent)
                : Core::Thread(64 * 1204, _T("HarwdareInitialization"))
                , _parent(parent)
                , _sleeptime(0)

            {
            }
            virtual ~Job()
            {
            }

        public:
            void Initialize(const uint32_t sleepTime)
            {
                _sleeptime = sleepTime;

                Core::Thread::Run();
            }

        private:
            virtual uint32_t Worker()
            {
                Block();

                SleepMs(_sleeptime);

                _parent.IsRunning();

                return Core::infinite;
            }

        private:
            PlatformImplementation& _parent;
            uint32_t _sleeptime;
        };

    public:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            enum enumSource {
                Internal,
                External
            };

        public:
            Config()
                : Core::JSON::Container()
                , Service(Internal)
                , HWDelay()
                , IRMode(NEXUS_IrInputMode_eCirNec)
                , Authentication(true)
                , BoxMode(1)
                , GraphicsHeap(0)
            {
                Add(_T("service"), &Service);
                Add(_T("hardwareready"), &HWDelay);
                Add(_T("irmode"), &IRMode);
                Add(_T("authentication"), &Authentication);
                Add(_T("boxmode"), &BoxMode);
                Add(_T("graphicsheap"), &GraphicsHeap);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::EnumType<enumSource> Service;
            Core::JSON::DecUInt16 HWDelay;
            Core::JSON::DecUInt16 IRMode;
            Core::JSON::Boolean Authentication;
            Core::JSON::DecUInt16 BoxMode;
            Core::JSON::DecUInt32 GraphicsHeap;
        };

    public:
        PlatformImplementation()
            : _hardwarestate(Exchange::IResourceCenter::UNITIALIZED)
            , _instance(nullptr)
            , _job(*this)
            , _resourceClients()
            , _compositionClients()
            , _mode(UNINITIALIZED)
            , _service()
        {
            // Register an @Exit, in case we are killed, with an incorrect ref count !!
            if (atexit(CloseDown) != 0) {
                TRACE(Trace::Information, (("Could not register @exit handler. Error: %d."), errno));
                exit(EXIT_FAILURE);
            }

            // Nexus can only be instantiated once (it is a process wide singleton !!!!)
            ASSERT(_implementation == nullptr);

            _identifier[0] = 0;
            _implementation = this;
        }
        ~PlatformImplementation()
        {
            if (_mode == INTERNAL_SERVER_RUNNING) {
                nxserver_ipc_uninit();
                nxserverlib_uninit(_instance);
                BKNI_DestroyMutex(_lock);
                NEXUS_Platform_Uninit();
            }
            else if (_mode == EXTERNAL_SERVER_JOINED) {
                NxClient_Uninit();
                BSTD_UNUSED(_instance);
            }

            _implementationLock.Lock();
            _implementation = nullptr;
            _implementationLock.Unlock();

            if (_service != nullptr)
                _service->Release();
        }

        /* virtual */ uint32_t Configure(PluginHost::IShell* service)
        {
            _service = service;
            _service->AddRef();

            uint32_t result = Core::ERROR_ILLEGAL_STATE;
            NEXUS_Error rc;
            PlatformImplementation::Config config;
            config.FromString(_service->ConfigLine());

            ASSERT(_instance == nullptr);

            _implementationLock.Lock();

            // Strange someone already started a NXServer.
            if (_instance == nullptr) {
                NxClient_GetDefaultJoinSettings(&(_joinSettings));
                strcpy(_joinSettings.name, "PlatformPlugin");

                if (config.Service.Value() == Plugin::PlatformImplementation::Config::enumSource::Internal) {
                    if (config.BoxMode.IsSet()) {
                        // Set box mode using env var.
                        std::stringstream boxMode;
                        boxMode << config.BoxMode.Value();
                        Core::SystemInfo::SetEnvironment(_T("B_REFSW_BOXMODE"), boxMode.str());
                    }
                    if (config.GraphicsHeap.IsSet() && config.GraphicsHeap.Value() > 0) {
                        // Set graphics heap size  using env var.
                        std::stringstream graphicsHeapSize;
                        graphicsHeapSize << config.GraphicsHeap.Value();
                        Core::SystemInfo::SetEnvironment(_T("NEXUS_GRAPHICS_HEAP_SIZE_BYTE"), graphicsHeapSize.str());
                    }

                    nxserver_get_default_settings(&(_serverSettings));
                    NEXUS_Platform_GetDefaultSettings(&(_platformSettings));
                    NEXUS_GetDefaultMemoryConfigurationSettings(&(_serverSettings.memConfigSettings));
                    NEXUS_GetPlatformCapabilities(&(_platformCapabilities));

                    /* display[1] will always be either SD or transcode */
                    if (!_platformCapabilities.display[1].supported || _platformCapabilities.display[1].encoder) {
                        for (unsigned int i = 0; i < NXCLIENT_MAX_SESSIONS; i++)
                            _serverSettings.session[i].output.sd = false;
                    }
                    /* display[0] will always be either HD, or system is headless */
                    if (!_platformCapabilities.display[0].supported || _platformCapabilities.display[0].encoder) {
                        for (unsigned int i = 0; i < NXCLIENT_MAX_SESSIONS; i++)
                            _serverSettings.session[i].output.hd = false;
                    }

#if NEXUS_HAS_IR_INPUT
                    for (unsigned int i = 0; i < NXCLIENT_MAX_SESSIONS; i++)
#if NEXUS_PLATFORM_VERSION_MAJOR < 16 || (NEXUS_PLATFORM_VERSION_MAJOR == 16 && NEXUS_PLATFORM_VERSION_MINOR < 3)
                        _serverSettings.session[i].ir_input_mode = static_cast<NEXUS_IrInputMode>(config.IRMode.Value());
#else
                        for (unsigned int irInputIndex = 0; i < NXSERVER_IR_INPUTS; i++)
                            _serverSettings.session[i].ir_input.mode[irInputIndex] = static_cast<NEXUS_IrInputMode>(config.IRMode.Value());
#endif // NEXUS_PLATFORM_VERSION_MAJOR < 17
#endif // NEXUS_HAS_IR_INPUT

                    struct nxserver_cmdline_settings cmdline_settings;
                    memset(&cmdline_settings, 0, sizeof(cmdline_settings));
                    cmdline_settings.frontend = true;
                    cmdline_settings.loudnessMode = NEXUS_AudioLoudnessEquivalenceMode_eNone;
                    _serverSettings.session[0].dolbyMs = nxserverlib_dolby_ms_type_none;

                    rc = nxserver_modify_platform_settings(&_serverSettings,
                        &cmdline_settings,
                        &_platformSettings,
                        &_serverSettings.memConfigSettings);
                    if (rc != NEXUS_SUCCESS) {
                        result = Core::ERROR_INCOMPLETE_CONFIG;
                    }
                    else {
                        rc = NEXUS_Platform_MemConfigInit(&_platformSettings, &_serverSettings.memConfigSettings);

                        if (rc != NEXUS_SUCCESS) {
                            result = Core::ERROR_UNKNOWN_KEY;
                        }
                        else {
                            BKNI_CreateMutex(&_lock);
                            _serverSettings.lock = _lock;

                            nxserver_set_client_heaps(&_serverSettings, &_platformSettings);

                            _serverSettings.client.connect = PlatformImplementation::ClientConnect;
                            _serverSettings.client.disconnect = PlatformImplementation::ClientDisconnect;

                            _instance = nxserverlib_init_extended(&_serverSettings, config.Authentication.Value());
                            if (_instance == nullptr) {
                                result = Core::ERROR_UNAVAILABLE;
                            }
                            else {
                                rc = nxserver_ipc_init(_instance, _lock);

                                if (rc != NEXUS_SUCCESS) {
                                    result = Core::ERROR_RPC_CALL_FAILED;
                                }
                                else {
                                    _mode = INTERNAL_SERVER_RUNNING;
                                    result = Core::ERROR_NONE;
                                    TRACE(Trace::Information, (_T("Creating internal NXServer.")));

                                    SetEventPlatformReady();
                                }
                            }
                        }
                    }
                }
                else {
                    result = Core::ERROR_RPC_CALL_FAILED;

                    TRACE(Trace::Information, (_T("Joining external NXServer.")));
                    rc = NxClient_Join(&_joinSettings);
                    if (rc == NEXUS_SUCCESS) {
                        _instance = nxserverlib_get_singleton();

                        if (_instance != nullptr) {
                            _mode = EXTERNAL_SERVER_JOINED;
                            result = Core::ERROR_NONE;

                            SetEventPlatformReady();
                        }
                    }

                    result = (_instance != nullptr ? Core::ERROR_NONE : Core::ERROR_RPC_CALL_FAILED);
                }
            }

            _implementationLock.Unlock();

            _hardwarestate = (result == Core::ERROR_NONE ? Exchange::IResourceCenter::INITIALIZING : Exchange::IResourceCenter::FAILURE);

            ASSERT(_hardwarestate != Exchange::IResourceCenter::FAILURE);

            if (_hardwarestate != Exchange::IResourceCenter::FAILURE) {

                // fake config.HWDelay.Value() miliseconds hardware initiation time.
                uint32_t sleep = config.HWDelay.Value();
                _job.Initialize(sleep * 1000);

                TRACE(Trace::Information, (_T("PlatformImplementation busy initializing (delay: %d)"), sleep));
            }

            return (result);
        }

        void SetEventPlatformReady()
        {
            // The platform appears to be ready, set the event
            PluginHost::ISubSystem* subSystem = _service->SubSystems();

            ASSERT(subSystem != nullptr);

            if (subSystem != nullptr) {
                subSystem->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
                subSystem->Release();
            }
        }

    public:
        BEGIN_INTERFACE_MAP(PlatformImplementation)
        INTERFACE_ENTRY(Exchange::IResourceCenter)
        INTERFACE_ENTRY(Exchange::IComposition)
        END_INTERFACE_MAP

    public:
        // -------------------------------------------------------------------------------------------------------
        //   IResourceCenter methods
        // -------------------------------------------------------------------------------------------------------
        /* virtual */ void Register(Exchange::IResourceCenter::INotification* notification)
        {
            // Do not double register a notification sink.
            _implementationLock.Lock();
            ASSERT(std::find(_resourceClients.begin(), _resourceClients.end(), notification) == _resourceClients.end());

            notification->AddRef();

            _resourceClients.push_back(notification);

            notification->StateChange(_hardwarestate);

            _implementationLock.Unlock();
        }

        /* virtual */ void Unregister(Exchange::IResourceCenter::INotification* notification)
        {
            _implementationLock.Lock();
            std::list<Exchange::IResourceCenter::INotification*>::iterator index(std::find(_resourceClients.begin(), _resourceClients.end(), notification));

            // Do not un-register something you did not register.
            ASSERT(index != _resourceClients.end());

            notification->Release();

            _resourceClients.erase(index);
            _implementationLock.Unlock();
        }

        /* virtual */ Exchange::IResourceCenter::hardware_state HardwareState() const
        {
            return (_hardwarestate);
        }

        /* virtual */ uint8_t Identifier(const uint8_t maxLength, uint8_t buffer[]) const
        {
            _implementationLock.Lock();

            if (_identifier[0] == 0) {
                NEXUS_OtpIdOutput id;

                if (NEXUS_SUCCESS != NEXUS_Security_ReadOtpId(NEXUS_OtpIdType_eA, &id)) {
                    // TODO: Why is this ASSERT triggered??
                    // ASSERT(false);
                }

                _identifier[0] = ((sizeof(_identifier) - 1) < id.size ? (sizeof(_identifier) - 1) : id.size);
                memcpy(&(_identifier[1]), id.otpId, _identifier[0]);
            }

            _implementationLock.Unlock();

            ::memcpy(buffer, &_identifier[1], (maxLength >= _identifier[0] ? _identifier[0] : maxLength));
            return (_identifier[0]);
        }

        // -------------------------------------------------------------------------------------------------------
        //   IComposition methods
        // -------------------------------------------------------------------------------------------------------
        /* virtual */ void Register(Exchange::IComposition::INotification* notification)
        {
            // Do not double register a notification sink.
            _implementationLock.Lock();
            ASSERT(std::find(_compositionClients.begin(), _compositionClients.end(), notification) == _compositionClients.end());

            notification->AddRef();

            _compositionClients.push_back(notification);

            std::list<Entry*>::iterator index(_nexusClients.begin());

            while (index != _nexusClients.end()) {

                if ((*index)->IsActive() == true) {
                    notification->Attached(*index);
                }
                index++;
            }

            _implementationLock.Unlock();
        }

        /* virtual */ void Unregister(Exchange::IComposition::INotification* notification)
        {
            _implementationLock.Lock();

            std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_compositionClients.begin(), _compositionClients.end(), notification));

            // Do not un-register something you did not register.
            ASSERT(index != _compositionClients.end());

            if (index != _compositionClients.end()) {

                _compositionClients.erase(index);

                notification->Release();
            }

            _implementationLock.Unlock();
        }

        /* virtual */ Exchange::IComposition::IClient* Client(const uint8_t id)
        {
            uint8_t count = id;
            Exchange::IComposition::IClient* result = nullptr;

            _implementationLock.Lock();

            std::list<Entry*>::iterator index(_nexusClients.begin());

            while ((count != 0) && (index != _nexusClients.end())) {
                if ((*index)->IsActive() == true)
                    count--;
                index++;
            }

            if (index != _nexusClients.end()) {
                result = (*index);
                result->AddRef();
            }

            _implementationLock.Unlock();

            return (result);
        }

        /* virtual */ Exchange::IComposition::IClient* Client(const string& name)
        {
            Exchange::IComposition::IClient* result = nullptr;

            _implementationLock.Lock();

            std::list<Entry*>::iterator index(_nexusClients.begin());

            while ((index != _nexusClients.end()) && ((*index)->Name() != name)) {
                index++;
            }

            if ((index != _nexusClients.end()) && ((*index)->IsActive() == true)) {
                result = (*index);
                result->AddRef();
            }

            _implementationLock.Unlock();

            return (result);
        }

    private:
        void IsRunning()
        {
            TRACE(Trace::Information, (_T("All hardware initialized")));

            _hardwarestate = Exchange::IResourceCenter::OPERATIONAL;

            Notify();
        }

        void RecursiveList(std::list<Exchange::IResourceCenter::INotification*>::iterator& index)
        {
            if (index == _resourceClients.end()) {

                _implementationLock.Unlock();
            }
            else {

                Exchange::IResourceCenter::INotification* callee(*index);

                ASSERT(callee != nullptr);

                callee->AddRef();

                RecursiveList(++index);

                callee->StateChange(_hardwarestate);
                callee->Release();
            }
        }

        void Notify()
        {
            _implementationLock.Lock();

            std::list<Exchange::IResourceCenter::INotification*>::iterator index(_resourceClients.begin());
            RecursiveList(index);
        }
        void Add(nxclient_t client, const NxClient_JoinSettings& joinSettings)
        {
            const std::string name(joinSettings.name);

            if (name.empty() == true) {
                TRACE(Trace::Information, (_T("Registration of a nameless client.")));
            }
            else {
                std::list<Entry*>::iterator index(_nexusClients.begin());

                while ((index != _nexusClients.end()) && (name != (*index)->Name())) {
                    index++;
                }

                if (index == _nexusClients.end()) {
                    Entry* entry(Entry::Create(client, joinSettings));

                    _nexusClients.push_back(entry);

                    TRACE(Trace::Information, (_T("Added client %s."), name.c_str()));

                    if (_compositionClients.size() > 0) {

                        std::list<Exchange::IComposition::INotification*>::iterator index(_compositionClients.begin());

                        while (index != _compositionClients.end()) {
                            (*index)->Attached(entry);
                            index++;
                        }
                    }
                }
                else {
                    TRACE(Trace::Information, (_T("Client already registered %s."), name.c_str()));
                }
            }
        }
        void Remove(const char clientName[])
        {
            const std::string name(clientName);

            if (name.empty() == true) {
                TRACE(Trace::Information, (_T("Registration of a nameless client.")));
            }
            else {
                std::list<Entry*>::iterator index(_nexusClients.begin());

                while ((index != _nexusClients.end()) && (name != (*index)->Name())) {
                    index++;
                }

                if (index != _nexusClients.end()) {
                    Entry* entry(*index);

                    _nexusClients.erase(index);

                    TRACE(Trace::Information, (_T("Removed client %s."), name.c_str()));

                    if (_compositionClients.size() > 0) {
                        std::list<Exchange::IComposition::INotification*>::iterator index(_compositionClients.begin());

                        while (index != _compositionClients.end()) {
                            (*index)->Detached(entry);
                            index++;
                        }
                    }

                    entry->Release();
                }
                else {
                    TRACE(Trace::Information, (_T("Client already unregistered %s."), name.c_str()));
                }
            }
        }
        static void CloseDown()
        {
            // Make sure we get exclusive access to the Destruction of this Resource Center.
            _implementationLock.Lock();

            // Seems we are destructed.....If we still have a pointer to the implementation, Kill it..
            if (_implementation != nullptr) {
                delete _implementation;
                _implementation = nullptr;
            }
            _implementationLock.Unlock();
        }

        static int ClientConnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings, NEXUS_ClientSettings* pClientSettings)
        {
            BSTD_UNUSED(pClientSettings);

            // Make sure we get exclusive access to the Resource Center.
            _implementationLock.Lock();

            if (_implementation != nullptr) {

                /* server app has opportunity to reject client altogether, or modify pClientSettings */
                _implementation->Add(client, *pJoinSettings);
            }

            _implementationLock.Unlock();

            return (0);
        }

        static void ClientDisconnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings)
        {
            BSTD_UNUSED(pJoinSettings);

            // Make sure we get exclusive access to the Resource Center.
            _implementationLock.Lock();

            if (_implementation != nullptr) {

                _implementation->Remove(pJoinSettings->name);
            }

            _implementationLock.Unlock();
        }

    private:
        hardware_state _hardwarestate;
        mutable uint8_t _identifier[16];
        BKNI_MutexHandle _lock;
        nxserver_t _instance;
        struct nxserver_settings _serverSettings;
        NEXUS_PlatformSettings _platformSettings;
        NEXUS_PlatformCapabilities _platformCapabilities;
        NxClient_JoinSettings _joinSettings;
        Job _job;
        std::list<Exchange::IResourceCenter::INotification*> _resourceClients;
        std::list<Exchange::IComposition::INotification*> _compositionClients;
        std::list<Entry*> _nexusClients;
        ServerMode _mode;
        PluginHost::IShell* _service;
    };

    SERVICE_REGISTRATION(PlatformImplementation, 1, 0);

} // namespace Plugin

    // ENUM_CONVERSION_BEGIN adds namespace Core, and the underlying type is defines in WPEFramework::Core
    ENUM_CONVERSION_BEGIN(Plugin::PlatformImplementation::Config::enumSource){ Plugin::PlatformImplementation::Config::enumSource::External, _TXT("external") },
    { Plugin::PlatformImplementation::Config::enumSource::Internal, _TXT("internal") },
    ENUM_CONVERSION_END(Plugin::PlatformImplementation::Config::enumSource)

} // namespace WPEFramework
