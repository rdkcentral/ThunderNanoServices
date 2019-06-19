#pragma once

#include <Module.h>
#include <interfaces/IStream.h>
#include "Geometry.h"

namespace WPEFramework {

namespace Player {

    namespace Implementation {

        using InitializerType = std::function<uint32_t(const string& configuration)>;
        using DeinitializerType = std::function<void()>;

        struct ICallback {
            virtual ~ICallback() {}

            virtual void TimeUpdate(uint64_t position) = 0;
            virtual void DRM(uint32_t state) = 0;
            virtual void StateChange(Exchange::IStream::state newState) = 0;
        };

        class IPlayerPlatform
        {
        public:
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

            virtual uint32_t AttachDecoder(const uint8_t index) = 0;
            virtual uint32_t DetachDecoder(const uint8_t index) = 0;
        };

        class IPlayerPlatformFactory
        {
        public:
            virtual ~IPlayerPlatformFactory() { }

            virtual uint32_t Initialize(const string& configuration) = 0;
            virtual void Deinitialize() = 0;

            virtual IPlayerPlatform* Create() = 0;
            virtual void Destroy(IPlayerPlatform* player) = 0;

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

            PlayerPlatformFactoryType(const string& name, Exchange::IStream::streamtype streamType, InitializerType initializer = nullptr, DeinitializerType deinitializer = nullptr)
                : _slots(nullptr)
                , _max_slots(0)
                , _name(name)
                , _streamType(streamType)
                , _Initialize(initializer)
                , _Deinitialize(deinitializer)
                , _configuration()
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

                if (Name().empty() == false) {
                    // Extract the particular player configuration
                    struct Config : public Core::JSON::Container {
                        Config(const string& tag) : Core::JSON::Container()
                        {
                            Add(tag.c_str(), &PlayerConfig);
                        }

                        Core::JSON::String PlayerConfig;
                    } config(Name());

                    config.FromString(configuration);
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
                }

                return (result);
            }

            void Deinitialize() override
            {
                if (_Deinitialize) {
                    _Deinitialize();
                }

                delete[] _slots;

                _slots = nullptr;
                _max_slots = 0;
            }

            IPlayerPlatform* Create() override
            {
                uint8_t index = 0;
                IPlayerPlatform* player = nullptr;

                while ((index < _max_slots) && (_slots[index] == true)) {
                    index++;
                }

                if (index < _max_slots) {
                    player =  new PLAYER(_streamType, index);
                    if (player != nullptr) {
                        _slots[index] = true;
                    }
                }

                return (player);
            }

            void Destroy(IPlayerPlatform* player) override
            {
                ASSERT(player != nullptr);

                uint8_t index = player->Index();
                ASSERT((index < _max_slots) &&  (_slots[index] == true));

                if ((index < _max_slots) && (_slots[index] == true)) {
                    _slots[index] = false;
                    delete player;
                }
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
        };

    } // namespace Implementation

} // namespace Player

}