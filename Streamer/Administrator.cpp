#include "Administrator.h"
#include "Frontend.h"

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
                , Decoders(0)
            {
                Add(_T("decoders"), &Decoders);
            }

        public:
            Core::JSON::DecUInt8 Decoders;
        };

        void Administrator::Announce(const string& name, IPlayerPlatformFactory* streamer)
        {
            ASSERT(name.empty() == false);
            ASSERT(streamer != nullptr);

            _adminLock.Lock();

            if (_streamers.emplace(name, streamer).second == false) {
                TRACE(Trace::Error, (_T("Failed to register streamer '%s' (type %i); streamer already registered!"),
                        name.c_str(), streamer->Type()));
            }

            _adminLock.Unlock();
        }

        void Administrator::Revoke(const string& name)
        {
            _adminLock.Lock();

            auto it = _streamers.find(name);
            if (it != _streamers.end()) {
                _streamers.erase(name);
            } else {
                TRACE(Trace::Error, (_T("Failed to unregister streamer '%s'"), name.c_str()));
            }

            _adminLock.Unlock();
        }

        uint32_t Administrator::Initialize(const string& configuration)
        {
            ASSERT(_decoder_slots == nullptr);
            ASSERT(_decoders == 0);

            Config config;
            config.FromString(configuration);

            _adminLock.Lock();

            for (auto& streamer : _streamers) {
                ASSERT(streamer.second != nullptr);

                if (streamer.second->Initialize(configuration) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to initialize streamer '%s' (type %i)"),
                            streamer.second->Name().c_str(), streamer.second->Type()));
                } else {
                    TRACE(Trace::Information, (_T("Initialized streamer '%s' (type %i, %i frontend(s) available)"),
                            streamer.second->Name().c_str(), streamer.second->Type(), streamer.second->Frontends()));
                }
            }

            _decoders = config.Decoders.Value();
            if (_decoders > 0) {
                _decoder_slots = new bool[_decoders];
                ASSERT(_decoder_slots != nullptr);

                if (_decoder_slots != nullptr) {
                    memset(_decoder_slots, 0, (sizeof(*_decoder_slots)*_decoders));
                }
            }

            TRACE(Trace::Information, (_T("Initialized stream administrator (%i decoder(s), %i streamer(s) available)"),
                    _decoders, _streamers.size()));

            _adminLock.Unlock();

            return (Core::ERROR_NONE);
        }

        uint32_t Administrator::Deinitialize()
        {
            _adminLock.Lock();

            for (auto& streamer : _streamers) {
                ASSERT(streamer.second != nullptr);

                TRACE(Trace::Information, (_T("Deinitializing streamer '%s'"), streamer.second->Name().c_str()));
                streamer.second->Deinitialize();
            }

            if (_decoders) {
                delete[] _decoder_slots;
                _decoder_slots = nullptr;
                _decoders = 0;
            }

            _adminLock.Unlock();

            TRACE(Trace::Information, (_T("Deinitialized stream administrator")));

            return (Core::ERROR_NONE);
        }

        Exchange::IStream* Administrator::Acquire(Exchange::IStream::streamtype streamType)
        {
            Frontend* frontend = nullptr;

            TRACE(Trace::Information, (_T("Looking for stream type %i player..."), streamType));

            _adminLock.Lock();

            auto it = _streamers.begin();
            for (; it != _streamers.end(); ++it) {
                ASSERT((*it).second != nullptr);
                if (((*it).second->Type() & streamType) != 0) {
                    IPlayerPlatform* player = (*it).second->Create();
                    if (player != nullptr) {
                        frontend = new Frontend(this, player);
                        ASSERT(frontend != nullptr);
                        if (frontend != nullptr) {
                            TRACE(Trace::Information, (_T("Acquired frontend '%s' for stream type %i at index %i"),
                                    (*it).second->Name().c_str(), streamType, frontend->Index()));
                        }
                    } else {
                        TRACE(Trace::Error, (_T("No more frontends available for stream type %i"), streamType));
                    }

                    break;
                }
            }

            if (it == _streamers.end()) {
                TRACE(Trace::Error, (_T("Stream type %i not suported!"), streamType));
            }

            _adminLock.Unlock();

            return (frontend);
        }

        void Administrator::Relinquish(IPlayerPlatform* player)
        {
            ASSERT(player != nullptr);

            _adminLock.Lock();

            auto it = _streamers.begin();
            for (; it != _streamers.end(); ++it) {
                ASSERT((*it).second != nullptr);
                if ((*it).second->Destroy(player) == true) {
                    TRACE(Trace::Information, (_T("Released frontend %s[%i]..."), (*it).second->Name().c_str(), player->Index()));
                    break;
                }
            }

            if (it == _streamers.end()) {
                ASSERT("Player instance not found");
                TRACE(Trace::Error, (_T("Failed to release a frontend")));
            }

            _adminLock.Unlock();
        }

        uint8_t Administrator::Allocate()
        {
            uint8_t index = 0;

            _adminLock.Lock();

            while ((index < _decoders) && (_decoder_slots[index] == true)) {
                index++;
            }

            if (index < _decoders) {
                _decoder_slots[index] = true;
                TRACE(Trace::Information, (_T("Allocated a decoder at slot %i"), index));
            } else {
                index = ~0;
                TRACE(Trace::Information, (_T("Failed to allocate a decoder; out of free decoders!")));
            }

            _adminLock.Unlock();

            return (index);
        }

        void Administrator::Deallocate(uint8_t index)
        {
            ASSERT(index < _decoders);

            _adminLock.Lock();

            if ((index < _decoders) && (_decoder_slots[index] == true)) {
                _decoder_slots[index] = false;
                TRACE(Trace::Information, (_T("Deallocated the decoder at slot %i"), index));
            } else {
                ASSERT(!"Decoder slot not allocated");
                TRACE(Trace::Error, (_T("Failed to deallocate a decoder; decoder slot not allocated")));
            }

            _adminLock.Unlock();
        }

    } // namespace Implementation

} // namespace Player

}