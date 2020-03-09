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
 
#include "Administrator.h"
#include "Designator.h"
#include <vector>

namespace WPEFramework {
namespace Player {
namespace Implementation {

    namespace {

        static Exchange::IStream::streamtype _supported;

        class QAM : public IPlayerPlatform {
        private:
            QAM() = delete;
            QAM(const QAM&) = delete;
            QAM& operator=(const QAM&) = delete;

            class Sink : public Broadcast::ITuner::ICallback {
            public:
                Sink() = delete;
                Sink(const Sink&) = delete;
                Sink& operator= (const Sink&) = delete;

                Sink (QAM& parent) : _parent(parent) {
                }
                ~Sink() {
                }

            private:
                void StateChange() override {
                    _parent.StateChange();
                }

            private:
                QAM& _parent;
            };

        public:
            QAM(const Exchange::IStream::streamtype streamType, const uint8_t index)
                : _state(Exchange::IStream::state::Error)
                , _drmType(Exchange::IStream::drmtype::Unknown)
                , _streamType(streamType)
                , _error(Core::ERROR_UNAVAILABLE)
                , _speed(0)
                , _absoluteTime(0)
                , _begin(0)
                , _end(~0)
                , _rectangle()
                , _z(0)
                , _callback(nullptr)
                , _player(nullptr)
                , _sink(*this)
                , _index(index)
            {
                _speeds.push_back(100);
            }
            ~QAM() override
            {
            }

        public:
            static Exchange::IStream::streamtype Supported() {
                return (_supported);
            }
            uint32_t Setup() override
            {
                uint32_t result = Core::ERROR_GENERAL;

                ASSERT(_state == Exchange::IStream::state::Error);

                _player = Broadcast::ITuner::Create(Core::NumberType<uint8_t>(_index).Text());
                if (_player != nullptr) {
                    _player->Callback(&_sink);
                    _state = Exchange::IStream::state::Idle;
                    result = Core::ERROR_NONE;
                    _error = result;
                }

                return result;
            }
            uint32_t Teardown() override
            {
                if (_player != nullptr) {
                    _player->Callback(nullptr);
                    delete _player;
                }

                _state = Exchange::IStream::state::Error;
                _error = Core::ERROR_UNAVAILABLE;

                return Core::ERROR_NONE;
            }
            void Callback(ICallback* callback) override
            {
                _callback = callback;
            }
            string Metadata() const override
            {
                return string();
            }
            Exchange::IStream::streamtype Type() const override
            {
                return _streamType;
            }
            Exchange::IStream::drmtype DRM() const override
            {
                return _drmType;
            }
            Exchange::IStream::state State() const override
            {
                return _state;
            }
            uint32_t Error() const override
            {
                return _error;
            }
            uint8_t Index() const override
            {
                return _index;
            }
            uint32_t Load(const string& configuration) override
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                ASSERT(_player != nullptr);

                result = Core::ERROR_ILLEGAL_STATE;

                if (_state != Exchange::IStream::state::Error) {

                    Broadcast::Designator parser(configuration);

                    TRACE(Trace::Information, (_T("Tuning to %u MHz mode=%s sym=%d Annex=%s spectralMode=%s"),
                        parser.Frequency(),
                        Core::EnumerateType<Broadcast::Modulation>(parser.Modulation()).Data(),
                        parser.SymbolRate(),
                        Core::EnumerateType<Broadcast::ITuner::annex>(_player->Annex()).Data(),
                        Core::EnumerateType<Broadcast::SpectralInversion>(parser.Spectral()).Data()));

                    result = _player->Tune(parser.Frequency(), parser.Modulation(),
                        parser.SymbolRate(), Broadcast::FEC_INNER_UNKNOWN, parser.Spectral());

                    if (result != Core::ERROR_NONE) {
                        _state = Exchange::IStream::state::Error;
                        TRACE(Trace::Error, (_T("Error in player load :%d"), result));
                        _error = result;
                        _callback->StateChange(_state);
                    } else {
                        TRACE(Trace::Information, (_T("Tuning to ProgramNumber %d"), parser.ProgramNumber()));
                        _player->Prepare(parser.ProgramNumber());
                        _state = Exchange::IStream::state::Prepared;
                    }
                }

                return result;
            }
            const std::vector<int32_t>& Speeds() const override
            {
                 return _speeds;
            }
            uint32_t Speed(const int32_t request) override
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                ASSERT(_player != nullptr);

                result = Core::ERROR_ILLEGAL_STATE;

                if ( (_state > Exchange::IStream::state::Prepared) && (_state != Exchange::IStream::state::Error) ) {
                    Exchange::IStream::state newState = _state;

                    result = Core::ERROR_NONE; // PLAYER_RESULT status =

                    // _player->setSpeed(request);
                    if (result == Core::ERROR_NONE) {
                        _speed = request;
                        newState = Exchange::IStream::state::Controlled;
                    } else {
                        TRACE(Trace::Error, (_T("Error in pause playback:%d"), result));
                        _error = result;
                        newState = Exchange::IStream::state::Error;
                    }

                    if ((newState != _state) && (_callback != nullptr)) {
                        _state = newState;
                        _callback->StateChange(_state);
                    }
                }

                return (result);
            }
            int32_t Speed() const override
            {
                return _speed;
            }
            void Position(const uint64_t absoluteTime) override
            {
                _absoluteTime = absoluteTime;
            }
            uint64_t Position() const override
            {
                return _absoluteTime;
            }
            void TimeRange(uint64_t& begin, uint64_t& end) const override
            {
                begin = _begin;
                end = _end;
            }
            const Rectangle& Window() const override
            {
                return (_rectangle);
            }
            void Window(const Rectangle& rectangle) override
            {
                _rectangle = rectangle;
            }
            uint32_t Order() const override
            {
                return (_z);
            }
            void Order(const uint32_t order) override
            {
                _z = order;
            }
            uint32_t AttachDecoder(const uint8_t index) override
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                ASSERT(_player != nullptr);

                result = Core::ERROR_ILLEGAL_STATE;

                if (_state == Exchange::IStream::state::Prepared) {

                    result = _player->Attach(index);
                    if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Error in attach decoder %d"), result));
                        _error = result;
                        _state = Exchange::IStream::state::Error;
                        _callback->StateChange(_state);
                    }
                }

                return (result);
            }
            uint32_t DetachDecoder(const uint8_t index) override
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                ASSERT(_player != nullptr);

                result = Core::ERROR_ILLEGAL_STATE;

                if ( (_state > Exchange::IStream::state::Prepared) && (_state != Exchange::IStream::state::Error) ) {

                    result = _player->Detach(index);
                    if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Error in detach decoder %d"), result));
                        _error = result;
                        _state = Exchange::IStream::state::Error;
                        _callback->StateChange(_state);
                    }
                }

                return (result);
            }
            const std::list<ElementaryStream>& Elements() const override
            {
                return _elements;
            }
            void StateChange() {
                ASSERT(_player != nullptr);

                Exchange::IStream::state oldState = _state;
                Broadcast::ITuner::state result = _player->State();

                if (result == Broadcast::ITuner::IDLE) {
                    _state = Exchange::IStream::state::Idle;
                }
                else if (result == Broadcast::ITuner::LOCKED) {
                    if (_state == Exchange::IStream::state::Idle) {
                        _state = Exchange::IStream::state::Loading;
                    }
                }
                else if (result == Broadcast::ITuner::PREPARED) {
                    if (_state == Exchange::IStream::state::Loading) {
                        _state = Exchange::IStream::state::Prepared;
                    }
                    else if (_state > Exchange::IStream::state::Prepared) {
                        _state = Exchange::IStream::state::Prepared;
                    }
                }
                else if (result == Broadcast::ITuner::STREAMING) {
                    _state = Exchange::IStream::state::Controlled;
                }

                if ( (oldState != _state) && (_callback != nullptr)) {
                    _callback->StateChange(_state);
                }
            }
            void StreamEvent(uint32_t eventId)
            {
                if (_callback != nullptr) {
                    _callback->StreamEvent(eventId);
                }
            }
            void PlayerEvent(uint32_t eventId)
            {
                if (_callback != nullptr) {
                    _callback->PlayerEvent(eventId);
                }
            }

        private:
            Exchange::IStream::state _state;
            Exchange::IStream::drmtype _drmType;
            Exchange::IStream::streamtype _streamType;

            uint32_t _error;

            int32_t _speed;
            std::vector<int32_t> _speeds;
            uint64_t _absoluteTime;
            uint64_t _begin;
            uint64_t _end;
            Rectangle _rectangle;
            uint32_t _z;

            std::list<ElementaryStream> _elements;

            ICallback* _callback;
            Broadcast::ITuner* _player;
            Sink _sink;
            uint8_t _index;
        };

        static PlayerPlatformRegistrationType<QAM, Exchange::IStream::streamtype::Undefined> Register(
            /*  Initialize */ [](const string& configuration) -> uint32_t {
                Broadcast::ITuner::Initialize(configuration);
                _supported = static_cast<Exchange::IStream::streamtype>(
                    (Broadcast::ITuner::IsSupported(Broadcast::ITuner::Cable)       ? static_cast<int>(Exchange::IStream::streamtype::Cable)       : 0) |
                    (Broadcast::ITuner::IsSupported(Broadcast::ITuner::Terrestrial) ? static_cast<int>(Exchange::IStream::streamtype::Terrestrial) : 0) |
                    (Broadcast::ITuner::IsSupported(Broadcast::ITuner::Satellite)   ? static_cast<int>(Exchange::IStream::streamtype::Satellite)   : 0) 
                );
                return (Core::ERROR_NONE);
            },
            /*  Deinitialize */ []() {
                Broadcast::ITuner::Deinitialize();
            }
        );

    } // namespace

} // namespace Implementation
} // namespace Player
}
