#include "PlayerImplementation.h"
#include "Administrator.h"

namespace WPEFramework {
namespace Player {
namespace Implementation {

    namespace {
        static PlayerPlatformRegistrationType<Bcast::PlayerPlatform> Register(PLAYER_NAME, Exchange::IStream::streamtype::RF,
            /*  Initialize */ [](const string& configuration) -> uint32_t {
                Broadcast::ITuner::Initialize(configuration);
                return (Core::ERROR_NONE);
            },
            /*  Deinitialize */ []() {
                Broadcast::ITuner::Deinitialize();
            }
        );
    }

} // namespace Implementation
} // namespace Player
}
