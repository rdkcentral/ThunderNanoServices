#include "Module.h"
#include "RemoteAdministrator.h"

#include <linux/uinput.h>
#include <interfaces/IKeyHandler.h>
#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nxclient.h>
#include <refsw/nexus_input_client.h>

extern int getIRHandle();

namespace WPEFramework {
namespace Plugin {

    class IRRemote : public Exchange::IKeyProducer {
    private:
        IRRemote(const IRRemote&) = delete;
        IRRemote& operator=(const IRRemote&) = delete;

        class Config : public Core::JSON::Container {
        private:
            Config (const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config ()
                : Core::JSON::Container()
                , CodeMask(static_cast<uint32_t>(~0))
            {
                Add(_T("codemask"), &CodeMask);
            }
            ~Config()
            {
            }

            Core::JSON::HexUInt32 CodeMask;
        };


    public:
        IRRemote();
        virtual ~IRRemote();

        BEGIN_INTERFACE_MAP(IRRemote)
            INTERFACE_ENTRY(Exchange::IKeyProducer)
        END_INTERFACE_MAP

        void Initialize();
        void Deinitialize();

        virtual const TCHAR* Name() const
        {
            return (_resourceName.c_str());
        }

        virtual void Configure(const string& configure)
        {
            Config config; config.FromString(configure);
            _codeMask = config.CodeMask.Value();
        }

        virtual bool Pair();
        virtual bool Unpair(string bindingId);
        virtual uint32_t Callback(Exchange::IKeyHandler* callback);
        virtual uint32_t Error() const;
        virtual string MetaData() const;

    private:
        static void NexusCallback(void* nexusIRHandle, int deviceID);
        void SendEvent();

    private:
        uint64_t _lastKeyTicks;
        uint32_t _error;
        Exchange::IKeyHandler* _callback;
            static const string _resourceName;
        NEXUS_InputClientHandle _NxInputClient;
        NxClient_AllocResults _allocResults;
        uint32_t _codeMask;
    };

    /* static */ const string IRRemote::_resourceName(_T("NexusIR"));
    static IRRemote* _singleton(Core::Service<IRRemote>::Create<IRRemote>());

    IRRemote::IRRemote()
        : _lastKeyTicks(0)
        , _error(Core::ERROR_UNAVAILABLE)
        , _callback(nullptr)
        , _NxInputClient(nullptr)
        , _allocResults()
        , _codeMask(static_cast<uint32_t>(~0))
    {

        Remotes::RemoteAdministrator::Instance().Announce(*this);
    }

    /* virtual */ IRRemote::~IRRemote() {

        Remotes::RemoteAdministrator::Instance().Revoke(*this);
    }

    void IRRemote::Initialize()
    {
        NEXUS_Error rc = NEXUS_SUCCESS;
        DEBUG_VARIABLE(rc);

        NxClient_AllocSettings allocSettings;
        NEXUS_InputClientSettings settings;

        rc = NxClient_Join(nullptr);
        ASSERT(rc == NEXUS_SUCCESS);

        NxClient_GetDefaultAllocSettings(&allocSettings);
        allocSettings.inputClient = 1;

        rc = NxClient_Alloc(&allocSettings, &_allocResults);
        ASSERT(rc == NEXUS_SUCCESS);

        _NxInputClient = NEXUS_InputClient_Acquire(_allocResults.inputClient[0].id);
        if (_NxInputClient) {
            TRACE_L1("%s: Acquired NEXUS Input Client", __FUNCTION__);
            NEXUS_InputClient_GetSettings(_NxInputClient, &settings);
            settings.filterMask = 1<<NEXUS_InputRouterDevice_eIrInput;

            settings.codeAvailable.callback = IRRemote::NexusCallback;
            settings.codeAvailable.param = _allocResults.inputClient[0].id;
            settings.codeAvailable.context = static_cast<NEXUS_InputClientHandle>(_NxInputClient);
            rc = NEXUS_InputClient_SetSettings(_NxInputClient, &settings);
            TRACE_L1("%s: NEXUS_InputClient_SetSettings rc=%d", __FUNCTION__, rc);
            _error = Core::ERROR_NONE;
        } else {
            _error = Core::ERROR_UNAVAILABLE;
            TRACE_L1("%s: NEXUS_InputClient_Acquire Failed. inputClient=%p", __FUNCTION__, _NxInputClient);
        }
    }

    void IRRemote::Deinitialize()
    {
        if (_NxInputClient) {
            NEXUS_InputClientSettings settings;
            NEXUS_InputClient_GetSettings(_NxInputClient, &settings);
            settings.codeAvailable.callback = nullptr;
            settings.codeAvailable.param = 0;
            settings.codeAvailable.context = nullptr;
            NEXUS_InputClient_SetSettings(_NxInputClient, &settings);
            NEXUS_InputClient_Release(_NxInputClient);
            NxClient_Free(&_allocResults);
            NxClient_Uninit();
        }

        _error = Core::ERROR_UNAVAILABLE;
    }

    /* virtual */ bool IRRemote::Pair() {
        return (false);
    }

    /* virtual */ bool IRRemote::Unpair(string bindingId) {
        return (false);
    }

    /* virtual */ uint32_t IRRemote::Callback(Exchange::IKeyHandler* callback) {

        ASSERT((callback == nullptr) ^ (_callback == nullptr));

        if (callback == nullptr) {
            // We are unlinked. Deinitialize the stuff.
            Deinitialize();
            _callback = nullptr;
        }
        else {

            Initialize();
            TRACE_L1("%s: callback=%p _callback=%p", __FUNCTION__, callback, _callback);
            _callback = callback;
        }

        return (Core::ERROR_NONE);
    }

    /* virtual */ uint32_t IRRemote::Error() const {
        return (_error);
    }

    /* virtual */ string IRRemote::MetaData() const {
        return _T("BCM Nexus");
    }

    /* static */ void IRRemote::NexusCallback(void* nexusIRHandle, int deviceID)
    {
        _singleton->SendEvent();
    }

    void IRRemote::SendEvent() {
        NEXUS_InputRouterCode inputRouterCode;
        unsigned num = 0;
        int rc;

        if (_NxInputClient) {
            // always get the key, else the queue fills up and weird things happen
            rc = NEXUS_InputClient_GetCodes((NEXUS_InputClientHandle) _NxInputClient, &inputRouterCode, 1, &num);
            int rawCode = inputRouterCode.data.irInput.code & _codeMask;
            int repeat = inputRouterCode.data.irInput.repeat;
            TRACE_L1("%s: RawCode 0x%X Codemask:0x%X Code:0x%X", __FUNCTION__, inputRouterCode.data.irInput.code, _codeMask, rawCode);
            if ((rc == 0) && (num != 0)) {

                uint64_t currentTicks = Core::Time::Now().Ticks();

                // See if this key is coming in more than 150ms aftre the last one..
                if (((currentTicks - _lastKeyTicks) < 150000) && (true == repeat)) {

                    TRACE_L1("%s: Ignoring key <150ms", __FUNCTION__);
                } else {
                    TRACE_L1("%s: sending keycode=%x", __FUNCTION__, rawCode);
                    _callback->KeyEvent(true, rawCode, _resourceName);
                    _callback->KeyEvent(false, rawCode, _resourceName);
                    _lastKeyTicks = currentTicks;
                }
            }
        } else {
            TRACE_L1("%s: Device is not connected", __FUNCTION__);
        }
    }
}
}
