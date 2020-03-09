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

#include <Module.h>
#include <interfaces/IStream.h>
#include "Geometry.h"
#include "Element.h"

#include <vector>
#include <set>

namespace WPEFramework {

namespace Player {

    namespace Implementation {

        using InitializerType = std::function<uint32_t(const string& configuration)>;
        using DeinitializerType = std::function<void()>;

        struct ICallback {
            virtual ~ICallback() { }

            virtual void TimeUpdate(uint64_t position) = 0;
            virtual void StateChange(Exchange::IStream::state newState) = 0;
            virtual void StreamEvent(uint32_t eventId) = 0;
            virtual void PlayerEvent(uint32_t eventId) = 0;
            virtual void DrmEvent(uint32_t state) = 0;
        };

        struct IPlayerPlatform {
            virtual ~IPlayerPlatform() { }

            virtual uint32_t Setup() = 0;
            virtual uint32_t Teardown() = 0;

            virtual void Callback(ICallback* callback) = 0;

            virtual string Metadata() const = 0;
            virtual Exchange::IStream::streamtype Type() const = 0;
            virtual Exchange::IStream::drmtype DRM() const = 0;
            virtual Exchange::IStream::state State() const = 0;
            virtual uint32_t Error() const = 0;
            virtual uint8_t Index() const = 0;

            virtual uint32_t Load(const string& uri) = 0;

            virtual uint32_t AttachDecoder(const uint8_t index) = 0;
            virtual uint32_t DetachDecoder(const uint8_t index) = 0;

            virtual uint32_t Speed(const int32_t speed) = 0;
            virtual int32_t Speed() const = 0;
            virtual const std::vector<int32_t>& Speeds() const = 0;

            virtual void Position(const uint64_t absoluteTime) = 0;
            virtual uint64_t Position() const = 0;

            virtual void TimeRange(uint64_t& begin, uint64_t& end) const = 0;

            virtual const Rectangle& Window() const = 0;
            virtual void Window(const Rectangle& rectangle) = 0;

            virtual uint32_t Order() const = 0;
            virtual void Order(const uint32_t order) = 0;

            virtual const std::list<ElementaryStream>& Elements() const = 0;
        };

        struct IPlayerPlatformFactory {
            virtual ~IPlayerPlatformFactory() { }

            virtual uint32_t Initialize(const string& configuration) = 0;
            virtual void Deinitialize() = 0;

            virtual IPlayerPlatform* Create() = 0;
            virtual bool Destroy(IPlayerPlatform* player) = 0;

            virtual const string& Name() const = 0;
            virtual Exchange::IStream::streamtype Type() const = 0;
            virtual uint8_t Frontends() const = 0;
            virtual const string& Configuration() const = 0;
        };

        template<class PLAYER, Exchange::IStream::streamtype STREAMTYPE>
        class PlayerPlatformFactoryType : public IPlayerPlatformFactory {
        public:
            PlayerPlatformFactoryType(const PlayerPlatformFactoryType&) = delete;
            PlayerPlatformFactoryType& operator=(const PlayerPlatformFactoryType&) = delete;

            PlayerPlatformFactoryType(InitializerType initializer = nullptr, DeinitializerType deinitializer = nullptr)
                : _slots()
                , _name(Core::ClassNameOnly(typeid(PLAYER).name()).Text())
                , _Initialize(initializer)
                , _Deinitialize(deinitializer)
                , _configuration()
                , _players()
                , _adminLock()
            {
                ASSERT(Name().empty() == false);
            }

            ~PlayerPlatformFactoryType()
            {
                ASSERT(_slots.Empty() == true);
            }

            uint32_t Initialize(const string& configuration) override
            {
                uint32_t result = Core::ERROR_GENERAL;

                // Extract the particular player configuration
                struct Config : public Core::JSON::Container {
                    Config(const string& tag) : Core::JSON::Container()
                    {
                        Add(tag.c_str(), &PlayerConfig);
                    }

                    Core::JSON::String PlayerConfig;
                } config(Name());

                config.FromString(configuration);

                _adminLock.Lock();
                ASSERT(_slots.Empty() == true);
                _configuration = config.PlayerConfig.Value();

                result = (_Initialize != nullptr? _Initialize(_configuration) : Core::ERROR_NONE);

                if (result == Core::ERROR_NONE) {
                    // Pick up the number of frontends
                    struct DefaultConfig : public Core::JSON::Container {
                    public:
                        DefaultConfig()
                            : Core::JSON::Container()
                            , Frontends(0)
                        {
                            Add(_T("frontends"), &Frontends);
                        }

                        Core::JSON::DecUInt8 Frontends;
                    } config;

                    config.FromString(_configuration);
                    _slots.Reset(config.Frontends.Value());
                }

                _adminLock.Unlock();

                return (result);
            }

            void Deinitialize() override
            {
                _adminLock.Lock();

                if (_Deinitialize) {
                    _Deinitialize();
                }

                _slots.Reset();

                _adminLock.Unlock();
            }

            IPlayerPlatform* Create() override
            {
                IPlayerPlatform* player = nullptr;

                _adminLock.Lock();

                uint8_t index = _slots.Find();
                if (index < _slots.Size()) {
                    player =  new PLAYER(Type(), index);

                    if ((player != nullptr) && (player->Setup() != Core::ERROR_NONE)) {
                        TRACE(Trace::Error, (_T("Player '%s' setup failed!"),  Name().c_str()));
                        delete player;
                        player = nullptr;
                    }

                    if (player != nullptr) {
                        _players.emplace(player);
                        _slots.Set(index);
                    }
                }

                _adminLock.Unlock();

                return (player);
            }

            bool Destroy(IPlayerPlatform* player) override
            {
                bool result = false;
                ASSERT(player != nullptr);

                _adminLock.Lock();

                auto it = _players.find(player);
                if (it != _players.end()) {
                    uint8_t index = player->Index();
                    ASSERT(_slots.IsSet(index) == true);

                    if (_slots.IsSet(index) == true) {
                        _slots.Clr(index);

                        if (player->Teardown() != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Player %s[%i] teardown failed!"), Name().c_str(), player->Index()));
                        }

                        _players.erase(it);
                        delete player;
                        result = true;
                    }
                }

                _adminLock.Unlock();

                return (result);
            }

            const string& Name() const override
            {
                return (_name);
            }

            Exchange::IStream::streamtype Type() const override
            {
                return (Type(TemplateIntToType<STREAMTYPE == Exchange::IStream::streamtype::Undefined>()));
            }

            uint8_t Frontends() const override
            {
                return (_slots.Size());
            }

            const string& Configuration() const override
            {
                return (_configuration);
            }

        private:
            Exchange::IStream::streamtype Type(const TemplateIntToType<true>& /* For compile time diffrentiation */) const 
            {
                // Lets load the StreamType from the Player..
                return (PLAYER::Supported());
            }
            Exchange::IStream::streamtype Type(const TemplateIntToType<false>& /* For compile time diffrentiation */) const 
            {
                return (STREAMTYPE);
            }

        private:
            Core::BitArrayFlexType<16> _slots;
            string _name;
            InitializerType _Initialize;
            DeinitializerType _Deinitialize;
            string _configuration;
            std::set<IPlayerPlatform*> _players;
            mutable Core::CriticalSection _adminLock;
        };

    } // namespace Implementation

} // namespace Player

}
