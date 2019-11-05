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
            ~Config() override
            {
            }

        public:
            Core::JSON::String Connector;
        } config;

        config.FromString(service->ConfigLine());

        _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance());
        _externalAccess = new ExternalAccess(Core::NodeId(config.Connector.Value().c_str()), this, service->ProxyStubPath(), _engine);

        result = Core::ERROR_OPENING_FAILED;
        if (_externalAccess != nullptr) {
            if (_externalAccess->IsListening() == false) {
                delete _externalAccess;
                _externalAccess = nullptr;
                _engine.Release();
            } else {
                result = _administrator.Initialize(service->ConfigLine());
            }
        }

        return (result);
    }

    /* virtual */ StreamerImplementation::~StreamerImplementation()
    {
        if (_externalAccess != nullptr) {

            _administrator.Deinitialize();

            TRACE(Trace::Information, (_T("StreamerImplementation::Destructor() : delete instance")));
            delete _externalAccess;
            _engine.Release();
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
