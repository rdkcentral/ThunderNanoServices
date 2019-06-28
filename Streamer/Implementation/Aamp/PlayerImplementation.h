#pragma once

#include "Module.h"
#include <chrono>
#include <interfaces/IStream.h>
#include <tracing/tracing.h>
#include <thread>
#include "PlayerPlatform.h"

#include <signal.h>
#include <stdlib.h>
#include <string>

class PlayerInstanceAAMP;
typedef struct _GMainLoop GMainLoop;

namespace WPEFramework {

namespace Player {

namespace Implementation {

    namespace Aamp {

        class AampEventListener;

        class PlayerPlatform : public IPlayerPlatform, Core::Thread {
        private:
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
            PlayerPlatform(const Exchange::IStream::streamtype streamType, const uint8_t index);
            virtual ~PlayerPlatform();

            uint32_t Setup() override;
            uint32_t Teardown() override;

            void Callback(ICallback* callback) override
            {
                _adminLock.Lock();
                _callback = callback;
                _adminLock.Unlock();
            }

            inline string Metadata() const override
            {
                return string();
            }
            inline Exchange::IStream::streamtype Type() const override
            {
                return _streamtype;
            }
            inline Exchange::IStream::drmtype DRM() const override
            {
                _adminLock.Lock();
                if (_drmType == Exchange::IStream::Unknown) {
                    const_cast<PlayerPlatform*>(this)->QueryDRMSystem();
                }
                Exchange::IStream::drmtype drmType = _drmType;
                _adminLock.Unlock();
                return drmType;
            }
            inline void StateChange(Exchange::IStream::state newState)
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

            inline Exchange::IStream::state State() const override
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

            uint32_t Load(const string& uri) override;

            const SpeedList& Speeds() const override
            {
                 return _speeds;
            }
            uint32_t Speed(const int32_t speed) override;
            inline int32_t Speed() const override
            {
                _adminLock.Lock();
                int32_t speed = _speed;
                _adminLock.Unlock();
                return speed;
            }
            void Position(const uint64_t absoluteTime) override;
            uint64_t Position() const override;
            void TimeUpdate();

            inline void TimeRange(uint64_t& begin, uint64_t& end) const override
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
            void Window(const Rectangle& rectangle) override;
            inline uint32_t Order() const override
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
            uint32_t AttachDecoder(const uint8_t index) override;
            uint32_t DetachDecoder(const uint8_t index) override;
            void Stop();
            void Terminate();
            void UpdateSpeed(int speed)
            {
                _adminLock.Lock();
                _speed = speed;
                _adminLock.Unlock();            }

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
            Exchange::IStream::streamtype _streamtype;

            SpeedList _speeds;
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
        };

    } // namespace Aamp

}// namespace Implementation

} // namespace Player

}
