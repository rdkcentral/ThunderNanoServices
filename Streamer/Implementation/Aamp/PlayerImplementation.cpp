#include "PlayerImplementation.h"
#include <gst/gst.h>
#include <main_aamp.h>

namespace WPEFramework {

namespace Player {

    namespace Implementation {

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
                   TRACE(Trace::Information, (_T("AAMP_EVENT_TUNED")));
                   break;
               case AAMP_EVENT_TUNE_FAILED:
                   _player->State(Exchange::IStream::Error);
                   _player->Stop();
                   TRACE(Trace::Information, (_T("AAMP_EVENT_TUNE_FAILED")));
                   break;
               case AAMP_EVENT_SPEED_CHANGED:
                   TRACE(Trace::Information, (_T("AAMP_EVENT_SPEED_CHANGED")));
                   break;
               case AAMP_EVENT_DRM_METADATA:
                   TRACE(Trace::Information, (_T("AAMP_DRM_FAILED")));
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

        class Config : public Core::JSON::Container {
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
        };

        string PlayerPlatform::_configuration = "";

        PlayerPlatform::PlayerPlatform(const Exchange::IStream::streamtype type, const uint8_t index, ICallback* callbacks)
            : _uri("")
            , _state(Exchange::IStream::Error)
            , _drmType(Exchange::IStream::Unknown)
            , _streamType(type)
            , _speed(-1)
            , _begin(0)
            , _end(~0)
            , _z(0)
            , _rectangle()
            , _callback(callbacks)
            , _initialized(false)
            , _aampEventListener(nullptr)
            , _aampGstPlayerMainLoop(nullptr)
            , _scheduler(this)
            , _adminLock()
        {
            Config config;
            config.FromString(_configuration);

            if ((config.Speeds.IsSet() == true) && (config.Speeds.Length() != 0)) {
                Core::JSON::ArrayType<Core::JSON::DecSInt32>::Iterator index(config.Speeds.Elements());
                while (index.Next() == true) {
                    _speeds.push_back(index.Current().Value());
                }
            }
            else {
                int32_t speeds[] = { 100, -100, 200, -200, 400, -400, 800, -800, 1600, -1600, 3200, -3200};
                _speeds.assign(std::begin(speeds), std::end(speeds));
            }
            _rectangle.X = 0;
            _rectangle.Y = 0;
            _rectangle.Width = 1080;
            _rectangle.Height = 720;

            if (config.WesterosSink.Value() == true) {
                Core::SystemInfo::SetEnvironment(_T("PLAYERSINKBIN_USE_WESTEROSSINK"), _T("true"));
            }

            gst_init(0, nullptr);

            _aampPlayer = new PlayerInstanceAAMP();
            ASSERT(_aampPlayer);

            _aampEventListener = new AampEventListener(this);
            _aampPlayer->RegisterEvents(_aampEventListener);

            _state = Exchange::IStream::Idle;
        }

        PlayerPlatform::~PlayerPlatform()
        {
            _scheduler.Quit();
            _speeds.clear();

            delete _aampPlayer;
            _aampPlayer = nullptr;

            delete _aampEventListener;
            _aampEventListener = nullptr;
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

        void PlayerPlatform::AttachDecoder(const uint8_t index)
        {
            InitializePlayerInstance();

            Run();
        }

        void PlayerPlatform::DetachDecoder(const uint8_t index)
        {
            Terminate();
        }

        void PlayerPlatform::Window(const Rectangle& rectangle)
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
                    _speed = -1;
                    _drmType = Exchange::IStream::Unknown;

                    _uri = uri;
                    _aampPlayer->Tune(_uri.c_str());
                    _aampPlayer->SetRate(0);

                    // TODO: wait for aaamp tune event before setting this
                    _state = Exchange::IStream::Prepared;
                    if (_callback != nullptr) {
                       _callback->StateChange(_state);
                    }
                    _adminLock.Unlock();
                } else {
                    result = Core::ERROR_INCORRECT_URL;
                    _adminLock.Lock();
                    _state = Exchange::IStream::Error;
                    _adminLock.Unlock();
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
                if (_state != newState) {
                    _state = newState;
                    if (_callback != nullptr) {
                       _callback->StateChange(_state);
                    }
                }
            }

            _adminLock.Unlock();
            return result;
       }
    }
}
} //namespace WPEFramework::Player::Implementation
