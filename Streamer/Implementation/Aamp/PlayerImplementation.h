#pragma once

#include "Module.h"
#include <interfaces/IStream.h>
#include <plugins/plugins.h>
#include <tracing/tracing.h>

#include <signal.h>
#include <stdlib.h>
#include <string>

class PlayerInstanceAAMP;
typedef struct _GMainLoop GMainLoop;

namespace WPEFramework {

namespace Player {

    namespace Implementation {
        class PlayerPlatform : public Core::Thread {
        private:
            PlayerPlatform() = delete;
            PlayerPlatform(const PlayerPlatform&) = delete;
            PlayerPlatform& operator=(const PlayerPlatform&) = delete;

        public:
            typedef std::vector<int32_t> SpeedList;
            static constexpr uint32_t TimeToGetPlaybackPosition = 1000;

        class Job: public Core::IDispatchType<void> {
        private:
            Job() = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

        public:
            Job(PlayerPlatform* player)
            : _parent(player)
            {
            }
            ~Job() {}

        private:
            virtual void Dispatch() override
            {
                Core::Time nextRun (Core::Time::Now());
                nextRun.Add(TimeToGetPlaybackPosition);
                PluginHost::WorkerPool::Instance().Schedule(nextRun, Core::ProxyType<Core::IDispatch>(*this));
                _parent->TimeUpdate();
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
                if (_drmType == Exchange::IStream::Unknown) {
                    const_cast<PlayerPlatform*>(this)->QueryDRMSystem();
                }
                return (Exchange::IStream::drmtype)_drmType;
            }
            inline Exchange::IStream::state State() const
            {
                return (Exchange::IStream::state)_state;
            }
            uint32_t Load(const string& uri);

            const SpeedList& Speeds() const
            {
                 return _speeds;
            }
            uint32_t Speed(const int32_t speed);
            inline int32_t Speed() const
            {
                return _speed;
            }
            void Position(const uint64_t absoluteTime);
            uint64_t Position() const;
            void TimeUpdate();

            inline void TimeRange(uint64_t& begin, uint64_t& end) const
            {
                begin = _begin;
                end = _end;
            }
            inline const Rectangle& Window() const
            {
                return (_rectangle);
            }
            void Window(const Rectangle& rectangle);
            inline uint32_t Order() const
            {
                return (_z);
            }
            inline void Order(const uint32_t order)
            {
                _z = order;
            }
            void AttachDecoder(const uint8_t index);
            void DetachDecoder(const uint8_t index);
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
            GMainLoop *_aampGstPlayerMainLoop;

            static string _configuration;
            Core::ProxyType<Job> _scheduler;
        };
    }
}
} // namespace WPEFramework::Player::Implementation
