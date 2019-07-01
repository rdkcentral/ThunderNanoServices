#pragma once

#include <Module.h>
#include <interfaces/IStream.h>
#include "Geometry.h"

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
            virtual void DRM(uint32_t state) = 0;
            virtual void StateChange(Exchange::IStream::state newState) = 0;
        };

        struct IPlayerPlatform
        {
            virtual ~IPlayerPlatform() { }

            virtual uint32_t Setup() = 0;
            virtual uint32_t Teardown() = 0;

            virtual void Callback(ICallback* callback) = 0;

            virtual string Metadata() const = 0;
            virtual Exchange::IStream::streamtype Type() const = 0;
            virtual Exchange::IStream::drmtype DRM() const = 0;
            virtual Exchange::IStream::state State() const = 0;
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
        };

        struct IPlayerPlatformFactory
        {
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

        template<class PLAYER>
        class PlayerPlatformFactoryType : public IPlayerPlatformFactory
        {
        public:
            PlayerPlatformFactoryType(const PlayerPlatformFactoryType&) = delete;
            PlayerPlatformFactoryType& operator=(const PlayerPlatformFactoryType&) = delete;

            PlayerPlatformFactoryType(Exchange::IStream::streamtype streamType, InitializerType initializer = nullptr, DeinitializerType deinitializer = nullptr)
                : _slots(nullptr)
                , _max_slots(0)
                , _name(Core::ClassNameOnly(typeid(PLAYER).name()).Text())
                , _streamType(streamType)
                , _Initialize(initializer)
                , _Deinitialize(deinitializer)
                , _configuration()
                , _players()
                , _adminLock()
            {
                ASSERT(Name().empty() == false);
                ASSERT(streamType != Exchange::IStream::streamtype::Undefined);
            }

            ~PlayerPlatformFactoryType()
            {
                delete[] _slots;
            }

            uint32_t Initialize(const string& configuration) override
            {
                ASSERT(_max_slots == 0);
                ASSERT(_slots == nullptr);

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
                    _max_slots = config.Frontends.Value();

                    if (_max_slots > 0) {
                        _slots = new bool[_max_slots];
                        ASSERT(_slots != nullptr);

                        if (_slots) {
                            memset(_slots, 0, (sizeof(*_slots) * _max_slots));
                        }
                    } else {
                        TRACE(Trace::Warning, (_T("No frontends defined for player '%s'"), Name().c_str()));
                    }
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

                delete[] _slots;

                _slots = nullptr;
                _max_slots = 0;

                _adminLock.Unlock();
            }

            IPlayerPlatform* Create() override
            {
                uint8_t index = 0;
                IPlayerPlatform* player = nullptr;

                _adminLock.Lock();

                while ((index < _max_slots) && (_slots[index] == true)) {
                    index++;
                }

                if (index < _max_slots) {
                    player =  new PLAYER(_streamType, index);

                    if ((player != nullptr) && (player->Setup() != Core::ERROR_NONE)) {
                        TRACE(Trace::Error, (_T("Player '%s' setup failed!"),  Name().c_str()));
                        delete player;
                        player = nullptr;
                    }

                    if (player != nullptr) {
                        _players.emplace(player);
                        _slots[index] = true;
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
                    ASSERT((index < _max_slots) &&  (_slots[index] == true));

                    if ((index < _max_slots) && (_slots[index] == true)) {
                        _slots[index] = false;

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
                return (_streamType);
            }

            uint8_t Frontends() const override
            {
                return (_max_slots);
            }

            const string& Configuration() const override
            {
                return (_configuration);
            }

        private:
            bool* _slots;
            uint8_t _max_slots;
            string _name;
            Exchange::IStream::streamtype _streamType;
            InitializerType _Initialize;
            DeinitializerType _Deinitialize;
            string _configuration;
            std::set<IPlayerPlatform*> _players;
            mutable Core::CriticalSection _adminLock;
        };

    } // namespace Implementation

} // namespace Player

}