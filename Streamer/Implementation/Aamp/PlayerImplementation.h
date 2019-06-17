#pragma once

#include "Module.h"
#include <chrono>
#include <interfaces/IStream.h>
#include <plugins/plugins.h>
#include <tracing/tracing.h>
#include <thread>

#include <signal.h>
#include <stdlib.h>
#include <string>

class PlayerInstanceAAMP;
typedef struct _GMainLoop GMainLoop;

namespace WPEFramework {

namespace Player {

    namespace Implementation {
        class AampEventListener;

        class PlayerPlatform : public Core::Thread {
        private:
            PlayerPlatform() = delete;
            PlayerPlatform(const PlayerPlatform&) = delete;
            PlayerPlatform& operator=(const PlayerPlatform&) = delete;

        public:
            typedef std::vector<int32_t> SpeedList;
            static constexpr uint32_t TimeToGetPlaybackPosition = 1;

        class Scheduler: public Core::Thread {
        private:
            Scheduler() = delete;
            Scheduler(const Scheduler&) = delete;
            Scheduler& operator=(const Scheduler&) = delete;

        public:
            Scheduler(PlayerPlatform* player)
            : _parent(player)
            {
            }
            ~Scheduler() {}

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
            PlayerPlatform* _parent;
        };

        public:
            PlayerPlatform(const Exchange::IStream::streamtype type, const uint8_t index, ICallback* callbacks);
            virtual ~PlayerPlatform();

        public:
            static uint32_t Initialize(const string& configuration)
            {
                _configuration = configuration;
                return (Core::ERROR_NONE);
            }
            static uint32_t Deinitialize()
            {
                return (Core::ERROR_NONE);
            }
            inline string Metadata() const
            {
                return string("{}");
            }
            inline Exchange::IStream::streamtype Type() const
            {
                return (Exchange::IStream::streamtype)_streamType;
            }
            inline Exchange::IStream::drmtype DRM() const
            {
                _adminLock.Lock();
                if (_drmType == Exchange::IStream::Unknown) {
                    const_cast<PlayerPlatform*>(this)->QueryDRMSystem();
                }
                Exchange::IStream::drmtype drmType = _drmType;
                _adminLock.Unlock();
                return drmType;
            }
            inline void State(Exchange::IStream::state curState)
            {
                _adminLock.Lock();
                _state = curState;
                _adminLock.Unlock();
            }

            inline Exchange::IStream::state State() const
            {
                _adminLock.Lock();
                Exchange::IStream::state curState = _state;
                _adminLock.Unlock();
                return curState;
            }
            uint32_t Load(const string& uri);

            const SpeedList& Speeds() const
            {
                 return _speeds;
            }
            uint32_t Speed(const int32_t speed);
            inline int32_t Speed() const
            {
                _adminLock.Lock();
                int32_t speed = _speed;
                _adminLock.Unlock();
                return speed;
            }
            void Position(const uint64_t absoluteTime);
            uint64_t Position() const;
            void TimeUpdate();

            inline void TimeRange(uint64_t& begin, uint64_t& end) const
            {
                _adminLock.Lock();
                begin = _begin;
                end = _end;
                _adminLock.Unlock();
            }
            inline const Rectangle& Window() const
            {
                return (_rectangle);
            }
            void Window(const Rectangle& rectangle);
            inline uint32_t Order() const
            {
                _adminLock.Lock();
                uint32_t z = _z;
                _adminLock.Unlock();
                return (z);
            }
            inline void Order(const uint32_t order)
            {
                _adminLock.Lock();
                _z = order;
                _adminLock.Unlock();
            }
            void AttachDecoder(const uint8_t index);
            void DetachDecoder(const uint8_t index);
            void Stop();
            void Terminate();

        private:
            virtual uint32_t Worker() override;
            void InitializePlayerInstance();
            void DeinitializePlayerInstance();
            void QueryDRMSystem();

            inline string UriType(const string& uri)
            {
                if (uri.find_last_of(".") != std::string::npos)
                    return uri.substr(uri.find_last_of(".") + 1);
                return "";
            }

        private:
            string _uri;

            Exchange::IStream::state _state;
            Exchange::IStream::drmtype _drmType;
            Exchange::IStream::streamtype _streamType;

            SpeedList _speeds;
            int32_t _speed;
            uint64_t _begin;
            uint64_t _end;
            uint32_t _z;
            Rectangle _rectangle;

            ICallback* _callback;

            bool _initialized;
            PlayerInstanceAAMP* _aampPlayer;
            AampEventListener *_aampEventListener;
            GMainLoop *_aampGstPlayerMainLoop;

            static string _configuration;
            Scheduler _scheduler;
            mutable Core::CriticalSection _adminLock;
        };
    }
}
} // namespace WPEFramework::Player::Implementation
