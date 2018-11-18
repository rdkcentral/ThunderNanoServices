#include "Frontend.h"
#include "PlayerImplementation.h"

namespace WPEFramework {

namespace Player {

namespace Implementation {

/* virtual */ Exchange::IStream* Administrator::Aquire(const Exchange::IStream::streamtype streamType) {

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
