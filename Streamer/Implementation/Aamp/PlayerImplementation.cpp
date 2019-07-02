
#include "Administrator.h"
#include <gst/gst.h>
#include <main_aamp.h>
#include <thread>
#include <vector>

namespace WPEFramework {
namespace Player {
namespace Implementation {

    namespace {

        static class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , Speeds()
                , WesterosSink(false)
            {
                Add(_T("speeds"), &Speeds);
                Add(_T("westerossink"), &WesterosSink);
            }

            Core::JSON::ArrayType<Core::JSON::DecSInt32> Speeds;
            Core::JSON::Boolean WesterosSink;
        } config;

        class Aamp : public IPlayerPlatform, Core::Thread {
        private:
            typedef struct _GMainLoop GMainLoop;
            static constexpr uint32_t TimeToGetPlaybackPosition = 1;

            class AampEventListener : public AAMPEventListener {
            public:
                AampEventListener() = delete;
                AampEventListener(const AampEventListener&) = delete;
                AampEventListener& operator=(const AampEventListener&) = delete;

                AampEventListener(Aamp* player)
                    : _player(player)
                {
                    ASSERT(_player != nullptr);
                }

                ~AampEventListener()
                {
                }

                void Event(const AAMPEvent& event)
                {
                    ASSERT(_player != nullptr);
                    switch (event.type)
                    {
                    case AAMP_EVENT_STATE_CHANGED:
                        TRACE(Trace::Information, (_T("AAMP_EVENT_STATE_CHANGED")));
                        switch (event.data.stateChanged.state)
                        {
                        case eSTATE_RELEASED:
                        case eSTATE_IDLE:
                        case eSTATE_STOPPED:
                        case eSTATE_COMPLETE:
                            _player->StateChange(Exchange::IStream::Idle);
                            break;
                        case eSTATE_PREPARING:
                            _player->StateChange(Exchange::IStream::Loading);
                            break;
                        case eSTATE_PREPARED:
                            _player->Speed(0);
                            _player->StateChange(Exchange::IStream::Prepared);
                            break;
                        case eSTATE_PAUSED:
                            _player->StateChange(Exchange::IStream::Paused);
                            break;
                        case eSTATE_PLAYING:
                            _player->StateChange(Exchange::IStream::Playing);
                            break;
                        case eSTATE_ERROR:
                            _player->StateChange(Exchange::IStream::Error);
                            break;
                        default:
                            break;
                        }
                        break;
                    case AAMP_EVENT_TUNED:
                        TRACE(Trace::Information, (_T("AAMP_EVENT_TUNED")));
                        break;
                    case AAMP_EVENT_TUNE_FAILED:
                        TRACE(Trace::Information, (_T("AAMP_EVENT_TUNE_FAILED")));
                        _player->StateChange(Exchange::IStream::Error);
                        /* this never seems to trigger... */
                        break;
                    case AAMP_EVENT_DRM_METADATA:
                        TRACE(Trace::Information, (_T("AAMP_EVENT_DRM_METADATA")));
                        break;
                    case AAMP_EVENT_EOS:
                        TRACE(Trace::Information, (_T("AAMP_EVENT_EOS")));
                        _player->Position(0);
                        _player->Speed(0);
                        break;
                    case AAMP_EVENT_PROGRESS:
                        /* TODO: Use this for progress notifications. */
                        break;
                    case AAMP_EVENT_SPEED_CHANGED:
                        TRACE(Trace::Information, (_T("AAMP_EVENT_SPEED_CHANGED")));
                        _player->UpdateSpeed(event.data.speedChanged.rate * 100);
                        break;
                    case AAMP_EVENT_BITRATE_CHANGED:
                        TRACE(Trace::Information, (_T("AAMP_EVENT_BITRATE_CHANGED")));
                        break;
                    default:
                        break;
                    }
                }

            private:
                Aamp* _player;
            };

            class Scheduler: public Core::Thread
            {
            public:
                Scheduler() = delete;
                Scheduler(const Scheduler&) = delete;
                Scheduler& operator=(const Scheduler&) = delete;

                Scheduler(Aamp* player)
                : _parent(player)
                {
                }

                ~Scheduler()
                {
                }

                void Quit()
                {
                    Block();
                    Wait(Thread::STOPPED | Thread::BLOCKED, Core::infinite);
                }

            private:
                virtual uint32_t Worker() override
                {
                    if (IsRunning() == true) {
                        std::this_thread::sleep_for(std::chrono::seconds(TimeToGetPlaybackPosition));
                        _parent->TimeUpdate();
                    }
                    return (Core::infinite);
                }

            private:
                Aamp* _parent;
            };

        public:
            Aamp() = delete;
            Aamp(const Aamp&) = delete;
            Aamp& operator=(const Aamp&) = delete;

            Aamp(const Exchange::IStream::streamtype streamType, const uint8_t index)
                : _uri("")
                , _state(Exchange::IStream::Error)
                , _drmType(Exchange::IStream::Unknown)
                , _streamtype(streamType)
                , _speed(-1)
                , _begin(0)
                , _end(~0)
                , _z(0)
                , _rectangle()
                , _index(index)
                , _callback(nullptr)
                , _initialized(false)
                , _aampPlayer(nullptr)
                , _aampEventListener(nullptr)
                , _aampGstPlayerMainLoop(nullptr)
                , _scheduler(this)
                , _adminLock()
            {
                ASSERT(_initialized == false)
                ASSERT(_aampPlayer == nullptr);
                ASSERT(_aampGstPlayerMainLoop == nullptr);

                if((config.Speeds.IsSet() == true) && (config.Speeds.IsNull() == false) && (config.Speeds.Length() != 0)) {
                    auto index(config.Speeds.Elements());
                    while (index.Next() == true) {
                        _speeds.push_back(index.Current().Value());
                    }
                } else {
                    int32_t speeds[] = { 100, -100, 200, -200, 400, -400, 800, -800, 1600, -1600, 3200, -3200};
                _speeds.assign(std::begin(speeds), std::end(speeds));
                }

                _rectangle.X = 0;
                _rectangle.Y = 0;
                _rectangle.Width = 1080;
                _rectangle.Height = 720;
            }

            ~Aamp()
            {
                ASSERT(_aampPlayer == nullptr);
                ASSERT(_aampEventListener == nullptr);

                _scheduler.Quit();
                _speeds.clear();
            }

            // IPlayerPlatform overrides

            uint32_t Setup() override
            {
                uint32_t result = Core::ERROR_GENERAL;
                gst_init(0, nullptr); // no op after first call anyway...

                _adminLock.Lock();
                ASSERT(_state == Exchange::IStream::Error);
                ASSERT(_aampPlayer == nullptr);
                ASSERT(_aampEventListener == nullptr);

                _aampPlayer = new PlayerInstanceAAMP();
                if (_aampPlayer != nullptr) {
                    _aampEventListener = new AampEventListener(this);
                    if (_aampEventListener != nullptr) {
                        _aampPlayer->RegisterEvents(_aampEventListener);
                        StateChange(Exchange::IStream::Idle);
                        result = Core::ERROR_NONE;
                    }
                    else {
                        delete _aampPlayer;
                        _aampPlayer = nullptr;
                    }
                }

                _adminLock.Unlock();

                return (result);
            }

            uint32_t Teardown() override
            {
                Terminate();

                _adminLock.Lock();

                _aampPlayer->Stop();
                _aampPlayer->RegisterEvents(nullptr);

                delete _aampEventListener;
                _aampEventListener = nullptr;

                delete _aampPlayer;
                _aampPlayer = nullptr;

                _state = Exchange::IStream::Error;

                _adminLock.Unlock();

                return Core::ERROR_NONE;
            }

            void Callback(ICallback* callback) override
            {
                _adminLock.Lock();
                _callback = callback;
                _adminLock.Unlock();
            }

            uint32_t AttachDecoder(const uint8_t index VARIABLE_IS_NOT_USED) override
            {
                InitializePlayerInstance();

                Run();

                return Core::ERROR_NONE;
            }

            uint32_t DetachDecoder(const uint8_t index VARIABLE_IS_NOT_USED) override
            {
                Terminate();

                return Core::ERROR_NONE;
            }

            string Metadata() const override
            {
                return string();
            }

            Exchange::IStream::streamtype Type() const override
            {
                return _streamtype;
            }

            Exchange::IStream::drmtype DRM() const override
            {
                _adminLock.Lock();
                if (_drmType == Exchange::IStream::Unknown) {
                    const_cast<Aamp*>(this)->QueryDRMSystem();
                }
                Exchange::IStream::drmtype drmType = _drmType;
                _adminLock.Unlock();
                return drmType;
            }

            Exchange::IStream::state State() const override
            {
                _adminLock.Lock();
                Exchange::IStream::state curState = _state;
                _adminLock.Unlock();
                return curState;
            }

            uint8_t Index() const override
            {
                return _index;
            }

            uint32_t Load(const string& uri) override
            {
                uint32_t result = Core::ERROR_NONE;
                TRACE(Trace::Information, (_T("URI = %s"), uri.c_str()));

                if (uri.empty() == false) {
                    TRACE(Trace::Information, (_T("uri = %s"), uri.c_str()));
                    string uriType = UriType(uri);
                    if ((uriType == "m3u8") || (uriType == "mpd")) {
                        TRACE(Trace::Information, (_T("URI type is %s"), uriType.c_str()));

                        _adminLock.Lock();
                        _speed = -1;
                        _drmType = Exchange::IStream::Unknown;
                        _uri = uri;
                        _aampPlayer->Tune(_uri.c_str());
                        _adminLock.Unlock();
                    } else {
                        result = Core::ERROR_INCORRECT_URL;
                        TRACE(Trace::Error, (_T("URI is not dash/hls")));
                    }
                } else {
                    result = Core::ERROR_INCORRECT_URL;
                    TRACE(Trace::Error, (_T("URI is not provided")));
                }

                return result;
            }

            int32_t Speed() const override
            {
                _adminLock.Lock();
                int32_t speed = _speed;
                _adminLock.Unlock();
                return speed;
            }

            uint32_t Speed(const int32_t speed) override
            {
                TRACE(Trace::Information, (_T("speed = %d"), speed));
                uint32_t result = Core::ERROR_NONE;

                _adminLock.Lock();
                if (speed != _speed) {
                    int rate = speed / 100;

                    if (rate != 0) {
                        auto index =  std::find(_speeds.begin(), _speeds.end(), speed);
                        if (index != _speeds.end()) {
                            _scheduler.Run();
                        } else {
                            result = Core::ERROR_BAD_REQUEST;
                        }
                    } else {
                        _scheduler.Block();
                    }

                    _aampPlayer->SetRate(rate);
                }

                _adminLock.Unlock();
                return result;
            }

            const std::vector<int32_t>& Speeds() const override
            {
                 return _speeds;
            }

            uint64_t Position() const override
            {
                uint64_t position = 0;
                _adminLock.Lock();
                position = (_aampPlayer->GetPlaybackPosition() * 1000);
                _adminLock.Unlock();
                return position;
            }

            void Position(const uint64_t absoluteTime) override
            {
                _adminLock.Lock();
                _aampPlayer->Seek(absoluteTime / 1000);
                _adminLock.Unlock();
            }

            void TimeRange(uint64_t& begin, uint64_t& end) const override
            {
                _adminLock.Lock();
                begin = _begin;
                end = _end;
                _adminLock.Unlock();
            }

            const Rectangle& Window() const override
            {
                return (_rectangle);
            }

            void Window(const Implementation::Rectangle& rectangle) override
            {
                _adminLock.Lock();
                _rectangle = rectangle;
                _aampPlayer->SetVideoRectangle(_rectangle.X, _rectangle.Y, _rectangle.Width, _rectangle.Height);
                _adminLock.Unlock();
            }

            uint32_t Order() const override
            {
                _adminLock.Lock();
                uint32_t z = _z;
                _adminLock.Unlock();
                return (z);
            }

            inline void Order(const uint32_t order) override
            {
                _adminLock.Lock();
                _z = order;
                _adminLock.Unlock();
            }

            // Thread overrides

            uint32_t Worker() override
            {
                if (_aampGstPlayerMainLoop) {
                    g_main_loop_run(_aampGstPlayerMainLoop); // blocks
                    g_main_loop_unref(_aampGstPlayerMainLoop);
                    _aampGstPlayerMainLoop = nullptr;
                }
                return WPEFramework::Core::infinite;
            }

            // Aamp methods

            void TimeUpdate()
            {
                _adminLock.Lock();
                if ((_callback != nullptr) && (_state == Exchange::IStream::Playing)) {
                    _callback->TimeUpdate(_aampPlayer->GetPlaybackPosition());
                }
                _adminLock.Unlock();
            }

            void UpdateSpeed(int speed)
            {
                _adminLock.Lock();
                _speed = speed;
                _adminLock.Unlock();
            }

            void StateChange(Exchange::IStream::state newState)
            {
                _adminLock.Lock();
                if (_state != newState) {
                    _state = newState;
                    if (_callback != nullptr) {
                        _callback->StateChange(_state);
                    }
                }
                _adminLock.Unlock();
            }

        private:
            void Stop()
            {
                Speed(0);
                _aampPlayer->Stop();
            }

            void Terminate()
            {
                _adminLock.Lock();
                if (_initialized == true) {
                    _adminLock.Unlock();

                    _scheduler.Block();

                    _aampPlayer->Stop();
                    Block();

                    DeinitializePlayerInstance();

                    Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
                } else {
                    _adminLock.Unlock();
                }
            }

            void InitializePlayerInstance()
            {
                _adminLock.Lock();
                if (!_initialized) {
                    _initialized = true;
                    _aampGstPlayerMainLoop = g_main_loop_new(nullptr, false);
                }
                _adminLock.Unlock();
            }

            void DeinitializePlayerInstance()
            {
                _adminLock.Lock();
                if (_initialized == true) {
                    if (_aampGstPlayerMainLoop)
                        g_main_loop_quit(_aampGstPlayerMainLoop);
                    _initialized = false;
                }
                _adminLock.Unlock();
            }

            void QueryDRMSystem()
            {
                string drm = _aampPlayer->GetCurrentDRM();
                if (drm.empty() != true) {
                    if (!strcmp(drm.c_str(), "WideVine")) {
                        _drmType = Exchange::IStream::Widevine;
                    } else if (!strcmp(drm.c_str(), "PlayReady")) {
                        _drmType = Exchange::IStream::PlayReady;
                    } else {
                        _drmType = Exchange::IStream::Unknown;
                    }
                } else {
                    _drmType = Exchange::IStream::None;
                }
            }

            string UriType(const string& uri)
            {
                if (uri.find_last_of(".") != std::string::npos)
                    return uri.substr(uri.find_last_of(".") + 1);
                return "";
            }

        private:
            string _uri;

            Exchange::IStream::state _state;
            Exchange::IStream::drmtype _drmType;
            Exchange::IStream::streamtype _streamtype;

            std::vector<int32_t> _speeds;
            int32_t _speed;
            uint64_t _begin;
            uint64_t _end;
            uint32_t _z;
            Rectangle _rectangle;
            uint8_t _index;

            ICallback* _callback;

            bool _initialized;
            PlayerInstanceAAMP* _aampPlayer;
            AampEventListener *_aampEventListener;
            GMainLoop *_aampGstPlayerMainLoop;

            Scheduler _scheduler;
            mutable Core::CriticalSection _adminLock;
        }; // class Aamp

        static PlayerPlatformRegistrationType<Aamp> Register(Exchange::IStream::streamtype::Unicast,
            /*  Initialize */ [](const string& configuration) -> uint32_t {
                config.FromString(configuration);
                if (config.WesterosSink.Value() == true) {
                    Core::SystemInfo::SetEnvironment(_T("PLAYERSINKBIN_USE_WESTEROSSINK"), _T("true"));
                }
                return (Core::ERROR_NONE);
            });

    }  // namespace

} // namespace Implementation
} // namespace Player
}
