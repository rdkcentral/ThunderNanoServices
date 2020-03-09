/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
                , _slots()
            {
            }

            ~Administrator()
            {
                ASSERT(_slots.Empty() == true);
            }

        public:
            static Administrator& Instance() {
                static Administrator instance;
                return instance;
            }

            uint32_t Initialize(const string& config);
            uint32_t Deinitialize();

            void Announce(const string& name, IPlayerPlatformFactory* factory);
            void Revoke(const string& name);

            Exchange::IStream* Acquire(const Exchange::IStream::streamtype streamType);
            void Relinquish(IPlayerPlatform* player);

            uint8_t Allocate();
            void Deallocate(uint8_t index);

        private:
            Core::CriticalSection _adminLock;
            std::map<string, IPlayerPlatformFactory*> _streamers;
            Core::BitArrayFlexType<16> _slots;
        };

        template<class PLAYER, const Exchange::IStream::streamtype STREAMTYPE>
        class PlayerPlatformRegistrationType {
        public:
            PlayerPlatformRegistrationType() = delete;
            PlayerPlatformRegistrationType(const PlayerPlatformRegistrationType<PLAYER,STREAMTYPE>&) = delete;
            PlayerPlatformRegistrationType& operator=(const PlayerPlatformRegistrationType<PLAYER,STREAMTYPE>&) = delete;

            PlayerPlatformRegistrationType(
                    InitializerType initializer = nullptr, DeinitializerType deinitializer = nullptr)
            {
                string name(Core::ClassNameOnly(typeid(PLAYER).name()).Text());

                IPlayerPlatformFactory* factory = new PlayerPlatformFactoryType<PLAYER, STREAMTYPE>(initializer, deinitializer);
                ASSERT(factory != nullptr);

                if (factory) {
                    Administrator::Instance().Announce(name, factory);
                }
            }

            ~PlayerPlatformRegistrationType()
            {
                string name(Core::ClassNameOnly(typeid(PLAYER).name()).Text());

                Administrator::Instance().Revoke(name);
            }
        };

    } // namespace Implementation

 } // namespace Player

}
