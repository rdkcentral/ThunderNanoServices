#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME SimpleWorker
#endif

#include <core/core.h>

namespace Thunder {
namespace Core {
    class EXTERNAL SimpleWorker {
    public:
        struct EXTERNAL ICallback {
            virtual ~ICallback() = default;

            virtual uint64_t Activity() = 0;
        };

    private:
        class Dispatcher {
        public:
            Dispatcher() = delete;
            Dispatcher& operator=(const Dispatcher&) = delete;

            Dispatcher(ICallback* callback)
                : _callback(callback)
            {
            }
            Dispatcher(const Dispatcher& copy)
                : _callback(copy._callback)
            {
            }
            ~Dispatcher() = default;

        public:
            void Clear()
            {
                _callback = nullptr;
            }
            void Callback(ICallback* callback)
            {
                ASSERT((callback == nullptr) ^ (_callback == nullptr));
                _callback = callback;
            }
            bool operator==(const Dispatcher& rhs) const
            {
                return (rhs._callback == _callback);
            }
            bool operator!=(const Dispatcher& rhs) const
            {
                return (!operator==(rhs));
            }
            uint64_t Timed(const uint64_t /*time*/)
            {
                ASSERT(_callback != nullptr);
                return (_callback->Activity());
            }

        private:
            ICallback* _callback;
        };

        friend Core::SingletonType<SimpleWorker>;
        SimpleWorker();

    public:
        SimpleWorker(const SimpleWorker&) = delete;
        SimpleWorker& operator=(const SimpleWorker&) = delete;
        ~SimpleWorker();

        static SimpleWorker& Instance()
        {
            return (Core::SingletonType<SimpleWorker>::Instance());
        }

    public:
        uint32_t Schedule(ICallback* callback, const Core::Time& callbackTime);
        inline uint32_t Submit(ICallback* callback)
        {
            return Schedule(callback, Core::Time::Now());
        }
        uint32_t Revoke(ICallback* callback);

    private:
        Core::TimerType<Dispatcher> _timer;
    };
}
}
