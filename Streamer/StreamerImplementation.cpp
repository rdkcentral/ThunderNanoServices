#include "StreamerImplementation.h"
#include "Frontend.h"
#include "PlayerImplementation.h" 

namespace WPEFramework {

namespace Plugin {

SERVICE_REGISTRATION(StreamerImplementation, 1, 0);

/* virtual */ uint32_t StreamerImplementation::Configure(PluginHost::IShell* service) {

    uint32_t result = Core::ERROR_NONE;

    Config config; 
    config.FromString(service->ConfigLine());

    _externalAccess = new ExternalAccess(Core::NodeId(config.Connector.Value().c_str()), this, service->ProxyStubPath());
    result = _externalAccess->Open(RPC::CommunicationTimeOut);
    if (result != Core::ERROR_NONE) {
        TRACE(Trace::Information, (_T("Could not open StreamerImplementation server.")));
        delete _externalAccess;
        _externalAccess = nullptr;
        result = Core::ERROR_OPENING_FAILED;
    }
    else {
        Player::Implementation::FrontendType< Player::Implementation::PlayerPlatform >::Initialize(service->ConfigLine());
        _administrator = new Player::Implementation::Administrator(config.Frontends.Value(), config.Decoders.Value());
        
    }
    return (result);
}

/* virtual */ StreamerImplementation::~StreamerImplementation()
{
    if (_externalAccess != nullptr) {

        Player::Implementation::FrontendType< Player::Implementation::PlayerPlatform >::Deinitialize();

        TRACE(Trace::Information, (_T("StreamerImplementation::Destructor() : delete instance")));
        delete _externalAccess;
        _externalAccess = nullptr;
    }
    if (_administrator != nullptr) {
        delete _administrator;
        _administrator = nullptr;
    }
}

/* virtual */ Exchange::IStream* StreamerImplementation::CreateStream(const Exchange::IStream::streamtype streamType) {

    Exchange::IStream* result = nullptr;

    _adminLock.Lock();

    if (_administrator != nullptr) {
        result = _administrator->Aquire(streamType);
    }

    _adminLock.Unlock();

    return (result);
}

} } // namespace WPEFramework::Plugin
