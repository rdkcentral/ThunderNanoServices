#pragma once

#include "Module.h"
#include <interfaces/IStream.h>
#include "PlayerPlatform.h"

namespace WPEFramework {

namespace Player {

    namespace Implementation {

        class Administrator {
        private:
            Administrator(const Administrator&) = delete;
            Administrator& operator=(const Administrator&) = delete;

            Administrator()
                : _adminLock()
                , _streamers()
                , _decoders(0)
                , _decoder_slots(nullptr)
            {
            }

        public:
            static Administrator& Instance() {
                static Administrator instance;
                return instance;
            }

            uint32_t Initialize(const string& config);
            uint32_t Deinitialize();

            void Announce(IPlayerPlatformFactory* streamer);
            void Revoke(IPlayerPlatformFactory* streamer);

            Exchange::IStream* Acquire(const Exchange::IStream::streamtype streamType);
            void Relinquish(IPlayerPlatform* player);

            uint8_t Allocate();
            void Deallocate(uint8_t index);

        private:
            Core::CriticalSection _adminLock;
            std::map<Exchange::IStream::streamtype, IPlayerPlatformFactory*> _streamers;
            uint8_t _decoders;
            bool* _decoder_slots;
        };

        template<class PLAYER>
        class PlayerPlatformRegistrationType {
        public:
            PlayerPlatformRegistrationType() = delete;
            PlayerPlatformRegistrationType(const PlayerPlatformRegistrationType<PLAYER>&) = delete;
            PlayerPlatformRegistrationType& operator=(const PlayerPlatformRegistrationType<PLAYER>&) = delete;

            PlayerPlatformRegistrationType(const string& name, Exchange::IStream::streamtype streamType,
                    InitializerType initializer = nullptr, DeinitializerType deinitializer = nullptr)
                : _factory(nullptr)
            {
                IPlayerPlatformFactory* factory = new PlayerPlatformFactoryType<PLAYER>(name, streamType, initializer, deinitializer);
                ASSERT(factory != nullptr);

                if (factory) {
                    Administrator::Instance().Announce(factory);
                    _factory = factory;
                }
            }

            ~PlayerPlatformRegistrationType()
            {
                ASSERT(_factory != nullptr);
                Administrator::Instance().Revoke(_factory);
            }

        private:
            IPlayerPlatformFactory* _factory;
        };

    } // namespace Implementation

 } // namespace Player

}