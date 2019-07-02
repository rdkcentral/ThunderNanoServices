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

        struct IPlayerPlatform {
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

        template<uint8_t MAXBITS>
        class BitArrayType
        {
        public:
            static_assert(MAXBITS <= 64, "MAXBITS is too big");
            using T = typename std::conditional<MAXBITS <= 8, std::uint8_t,
                            typename std::conditional<MAXBITS <= 16, std::uint16_t,
                                    typename std::conditional<MAXBITS <= 32, std::uint32_t, std::uint64_t>::type>::type>::type;

            BitArrayType(uint8_t max = 0, T initial = 0)
            {
                Reset(max, initial);
            }

            void Set(uint8_t index)
            {
                ASSERT(index < Size());
                _value |= ((1 << index) & ((1 << Size()) - 1));
            }

            void Clr(uint8_t index)
            {
                ASSERT(index < Size());
                _value &= ~(1 << index);
            }

            bool IsSet(uint8_t index) const
            {
                ASSERT(index < Size());
                return (_value & (1 << index));
            }

            uint8_t Size() const
            {
                return _max;
            }

            bool Empty() const
            {
                return (_value == 0);
            }

            void Reset(uint8_t max = 0, T initial = 0)
            {
                ASSERT(max <= MAXBITS);
                _max = max;
                if ((max == 0) ||  (max > MAXBITS)) {
                    _max = MAXBITS;
                }
                _value = (initial & ((1 << _max) - 1));
            }

            uint8_t Find() const
            {
                for (uint8_t i = 0; i < Size(); i++) {
                    if (!IsSet(i)) {
                        return i;
                    }
                }
                return (~0);
            }

        private:
            T _value;
            uint8_t _max;
        };

        template<class PLAYER>
        class PlayerPlatformFactoryType : public IPlayerPlatformFactory {
        public:
            PlayerPlatformFactoryType(const PlayerPlatformFactoryType&) = delete;
            PlayerPlatformFactoryType& operator=(const PlayerPlatformFactoryType&) = delete;

            PlayerPlatformFactoryType(Exchange::IStream::streamtype streamType, InitializerType initializer = nullptr, DeinitializerType deinitializer = nullptr)
                : _slots()
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
                    player =  new PLAYER(_streamType, index);

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
                return (_streamType);
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
            BitArrayType<16> _slots;
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