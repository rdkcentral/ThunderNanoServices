#include "StreamerImplementation.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(StreamerImplementation, 1, 0);

    /* virtual */ uint32_t StreamerImplementation::Configure(PluginHost::IShell* service)
    {

        uint32_t result = Core::ERROR_NONE;

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector(_T("/tmp/player"))
            {
                Add(_T("connector"), &Connector);
            }
            virtual ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
        } config;

        config.FromString(service->ConfigLine());

        _externalAccess = new ExternalAccess(Core::NodeId(config.Connector.Value().c_str()), this, service->ProxyStubPath(), Core::ProxyType<RPC::InvokeServer>::Create());
        result = _externalAccess->Open(RPC::CommunicationTimeOut);
        if (result != Core::ERROR_NONE) {
            TRACE(Trace::Information, (_T("Could not open StreamerImplementation server.")));
            delete _externalAccess;
            _externalAccess = nullptr;
            result = Core::ERROR_OPENING_FAILED;
        } else {
            result = _administrator.Initialize(service->ConfigLine());
        }
        return (result);
    }

    /* virtual */ StreamerImplementation::~StreamerImplementation()
    {
        if (_externalAccess != nullptr) {

            _administrator.Deinitialize();

            TRACE(Trace::Information, (_T("StreamerImplementation::Destructor() : delete instance")));
            delete _externalAccess;
            _externalAccess = nullptr;
        }
    }

    /* virtual */ Exchange::IStream* StreamerImplementation::CreateStream(const Exchange::IStream::streamtype streamType)
    {

        Exchange::IStream* result = nullptr;

        _adminLock.Lock();

        result = _administrator.Acquire(streamType);

        _adminLock.Unlock();

        return (result);
    }
}

}
