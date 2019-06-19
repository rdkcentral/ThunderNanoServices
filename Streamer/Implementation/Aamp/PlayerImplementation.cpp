#include "PlayerImplementation.h"
#include <gst/gst.h>
#include <main_aamp.h>
#include "Administrator.h"

namespace WPEFramework {

namespace Player {

namespace Implementation {

    namespace Aamp {

    class AampEventListener :public AAMPEventListener {
        private:
            AampEventListener() = delete;
            AampEventListener(const AampEventListener&) = delete;
            AampEventListener& operator=(const AampEventListener&) = delete;

        public:
           AampEventListener(PlayerPlatform* player)
           : _player(player)
           {
           }
           void Event(const AAMPEvent & e)
           {
               switch (e.type)
               {
               case AAMP_EVENT_TUNED:
                   _player->StateChange(Exchange::IStream::Prepared);
                   TRACE(Trace::Information, (_T("AAMP_EVENT_TUNED")));
                   break;
               case AAMP_EVENT_TUNE_FAILED:
                   _player->StateChange(Exchange::IStream::Error);
                   _player->Stop();
                   TRACE(Trace::Information, (_T("AAMP_EVENT_TUNE_FAILED")));
                   break;
               case AAMP_EVENT_SPEED_CHANGED:
                   TRACE(Trace::Information, (_T("AAMP_EVENT_SPEED_CHANGED")));
                   break;
               case AAMP_EVENT_DRM_METADATA:
                   TRACE(Trace::Information, (_T("AAMP_EVENT_DRM_METADATA")));
                   break;
               case AAMP_EVENT_EOS:
                   _player->Position(0);
                   _player->Speed(0);
                   TRACE(Trace::Information, (_T("AAMP_EVENT_EOS")));
                   break;
               case AAMP_EVENT_PLAYLIST_INDEXED:
                   TRACE(Trace::Information, (_T("AAMP_EVENT_PLAYLIST_INDEXED")));
                   break;
               case AAMP_EVENT_PROGRESS:
                   break;
               case AAMP_EVENT_CC_HANDLE_RECEIVED:
                   TRACE(Trace::Information, (_T("AAMP_EVENT_CC_HANDLE_RECEIVED")));
                   break;
               case AAMP_EVENT_BITRATE_CHANGED:
                   TRACE(Trace::Information, (_T("AAMP_EVENT_BITRATE_CHANGED")));
                   break;
               default:
                   break;
              }
           }
        private:
            PlayerPlatform* _player;
        };

        static class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Speeds()
                , WesterosSink(false)
            {
                Add(_T("speeds"), &Speeds);
                Add(_T("westerossink"), &WesterosSink);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::ArrayType<Core::JSON::DecSInt32> Speeds;
            Core::JSON::Boolean WesterosSink;
        } config;

        PlayerPlatform::PlayerPlatform(const Exchange::IStream::streamtype streamType, const uint8_t index)
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

        PlayerPlatform::~PlayerPlatform()
        {
            _scheduler.Quit();
            _speeds.clear();
        }

        uint32_t PlayerPlatform::Setup()
        {
            uint32_t result = Core::ERROR_GENERAL;
            gst_init(0, nullptr); // no op after first call anyway...

            _adminLock.Lock();
            ASSERT(_state == Exchange::IStream::Error);

            _aampPlayer = new PlayerInstanceAAMP();
            if (_aampPlayer != nullptr) {
                _aampEventListener = new AampEventListener(this);
                if (_aampEventListener != nullptr) {
                    _aampPlayer->RegisterEvents(_aampEventListener);
                    _state = Exchange::IStream::Idle;
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

        uint32_t PlayerPlatform::Teardown()
        {
            Terminate();

            _adminLock.Lock();

            _aampPlayer->RegisterEvents(nullptr);

            delete _aampEventListener;
            _aampEventListener = nullptr;

            delete _aampPlayer;
            _aampPlayer = nullptr;

            _state = Exchange::IStream::Error;

            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }


        void PlayerPlatform::InitializePlayerInstance()
        {
            _adminLock.Lock();
            if (!_initialized) {
                _initialized = true;
                _aampGstPlayerMainLoop = g_main_loop_new(nullptr, false);
            }
            _adminLock.Unlock();
        }

        void PlayerPlatform::DeinitializePlayerInstance()
        {
            _adminLock.Lock();
            if (_initialized == true) {
                if (_aampGstPlayerMainLoop)
                    g_main_loop_quit(_aampGstPlayerMainLoop);
                _initialized = false;
            }
            _adminLock.Unlock();
        }

        uint32_t PlayerPlatform::AttachDecoder(const uint8_t index)
        {
            InitializePlayerInstance();

            Run();

            return 0;
        }

        uint32_t PlayerPlatform::DetachDecoder(const uint8_t index)
        {
            Terminate();
            return 0;
        }

        void PlayerPlatform::Window(const Implementation::Rectangle& rectangle)
        {
            _adminLock.Lock();
            _rectangle = rectangle;
            _aampPlayer->SetVideoRectangle(_rectangle.X, _rectangle.Y, _rectangle.Width, _rectangle.Height);
            _adminLock.Unlock();
        }

        void PlayerPlatform::QueryDRMSystem()
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

        uint32_t PlayerPlatform::Load(const string& uri)
        {
            uint32_t result = Core::ERROR_NONE;
            TRACE(Trace::Information, (string(__FUNCTION__)));
            TRACE(Trace::Information, (_T("uri = %s"), uri.c_str()));

            if (uri.empty() == false) {
                TRACE(Trace::Information, (_T("uri = %s"), uri.c_str()));
                string uriType = UriType(uri);
                if ((uriType == "m3u8") || (uriType == "mpd")) {
                    TRACE(Trace::Information, (_T("URI type is %s"), uriType.c_str()));

                    _adminLock.Lock();
                    StateChange(Exchange::IStream::Loading);
                    _speed = -1;
                    _drmType = Exchange::IStream::Unknown;

                    _uri = uri;
                    _aampPlayer->Tune(_uri.c_str());
                    _aampPlayer->SetRate(0);

                    _adminLock.Unlock();
                } else {
                    result = Core::ERROR_INCORRECT_URL;
                    StateChange(Exchange::IStream::Error);
                    TRACE(Trace::Error, (_T("URI is not dash/hls")));
                }
            } else {
                result = Core::ERROR_INCORRECT_URL;
                TRACE(Trace::Error, (_T("URI is not provided")));
            }

            return result;
        }

        uint64_t PlayerPlatform::Position() const
        {
            uint64_t position = 0;
            _adminLock.Lock();
            position = (_aampPlayer->GetPlaybackPosition() * 1000);
            _adminLock.Unlock();
            return position;
        }

        void PlayerPlatform::Position(const uint64_t absoluteTime)
        {
            _adminLock.Lock();
            _aampPlayer->Seek(absoluteTime/1000);
            _adminLock.Unlock();
        }

        void PlayerPlatform::TimeUpdate()
        {
            _adminLock.Lock();
            if ((_callback != nullptr) && (_state == Exchange::IStream::Playing)) {
                _callback->TimeUpdate(_aampPlayer->GetPlaybackPosition());
            }
            _adminLock.Unlock();
        }

        void PlayerPlatform::Stop()
        {
            Speed(0);
            _aampPlayer->Stop();
        }

        void PlayerPlatform::Terminate()
        {
            _adminLock.Lock();
            if (_initialized == true) {
                _adminLock.Unlock();

                _scheduler.Block();

                _aampPlayer->Stop();
                Block();

                DeinitializePlayerInstance();
                StateChange(Exchange::IStream::Prepared);

                TRACE(Trace::Information, (string(__FUNCTION__)));
                Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
            } else {
                _adminLock.Unlock();
            }
        }

        uint32_t PlayerPlatform::Worker()
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            if (_aampGstPlayerMainLoop) {
                g_main_loop_run(_aampGstPlayerMainLoop); // blocks
                TRACE(Trace::Information, (string(__FUNCTION__)));
                g_main_loop_unref(_aampGstPlayerMainLoop);
                _aampGstPlayerMainLoop = nullptr;
            }
            return WPEFramework::Core::infinite;
        }

        uint32_t PlayerPlatform::Speed(const int32_t speed)
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            TRACE(Trace::Information, (_T("speed = %d"), speed));
            uint32_t result = Core::ERROR_NONE;

            _adminLock.Lock();
            if (speed != _speed) {

                Exchange::IStream::state newState = _state;

                if (speed != 0) {
                    SpeedList::iterator index =  std::find(_speeds.begin(), _speeds.end(), speed);
                    if (index != _speeds.end()) {
                        newState = Exchange::IStream::Playing;
                        _scheduler.Run();
                    } else {
                        result = Core::ERROR_BAD_REQUEST;
                    }

                } else {

                    newState = Exchange::IStream::Paused;
                    _scheduler.Block();
                }
                _speed = speed;

                _aampPlayer->SetRate((speed/100));

                StateChange(newState);
            }

            _adminLock.Unlock();
            return result;
       }

    } // namespace Aamp

    namespace {
        static PlayerPlatformRegistrationType<Aamp::PlayerPlatform> Register(PLAYER_NAME, Exchange::IStream::streamtype::Unicast,
            /*  Initialize */ [](const string& configuration) -> uint32_t {
                Aamp::config.FromString(configuration);
                if (Aamp::config.WesterosSink.Value() == true) {
                    Core::SystemInfo::SetEnvironment(_T("PLAYERSINKBIN_USE_WESTEROSSINK"), _T("true"));
                }
                return (Core::ERROR_NONE);
            });
    }

} // namespace Implementation

} // namespace Player

}
