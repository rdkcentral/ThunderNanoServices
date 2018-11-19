#include "Frontend.h"
#include "PlayerImplementation.h"

namespace WPEFramework {

namespace Player {

namespace Implementation {

class Config : public Core::JSON::Container {
private:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

public:
    Config()
        : Core::JSON::Container()
        , Frontends(1)
        , Decoders(1)
    {
        Add(_T("frontends"), &Frontends);
        Add(_T("decoders"), &Decoders);
    }
    ~Config()
    {
    }

public:
    Core::JSON::DecUInt8 Frontends;
    Core::JSON::DecUInt8 Decoders;
};

uint32_t Administrator::Initialize(const string& configuration) {
    Config config; config.FromString(configuration);

    _frontends = config.Frontends.Value();
    _decoders = config.Decoders.Value();

    uint32_t bufferSize = (sizeof(Exchange::IStream*) * _frontends) + _decoders;
    void* buffer = ::malloc(bufferSize);

    ::memset(buffer, 0, bufferSize);

    _streams = reinterpret_cast<Exchange::IStream**>(buffer);
    _slots   = &(reinterpret_cast<uint8_t*>(buffer)[sizeof(Exchange::IStream*) * _frontends]);

    return (FrontendType< PlayerPlatform >::Initialize(configuration));
}

uint32_t Administrator::Deinitialize() {
    return(FrontendType< PlayerPlatform >::Deinitialize());
}

Exchange::IStream* Administrator::Aquire(const Exchange::IStream::streamtype streamType) {

    uint8_t index = 0;
    Exchange::IStream* result = nullptr;

    _adminLock.Lock();

    while ((index < _frontends) && (_streams[index] != nullptr)) { index++; }

    if (index < _frontends) {
        result = new FrontendType< PlayerPlatform > (this, streamType, index);

        _streams[index] = result;
    }

    _adminLock.Unlock();

    return (result);
}

} } } // namespace WPEFramework::Player::Implementation
