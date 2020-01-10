#pragma once

namespace WPEFramework {
namespace Core {

    template <typename IMPLEMENTATION>
    class EXTERNAL WorkerJob : public Core::IDispatch {
    public:
        WorkerJob() = delete;
        WorkerJob(const WorkerJob&) = delete;
        WorkerJob& operator=(const WorkerJob&) = delete;

    public:
        WorkerJob(IMPLEMENTATION* parent)
            : _pendingJob(false)
            , _implementation(*parent)
            , _lock(false)
        {
        }
        virtual ~WorkerJob()
        {
            PluginHost::WorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(*this));
        }

    public:
        void Submit()
        {
            while (_lock.exchange(true, std::memory_order_relaxed));
            if (_pendingJob != true) {
                PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                _pendingJob = true;
            }
            _lock.store(false, std::memory_order_relaxed);
        }

    private:
        virtual void Dispatch()
        {
            _implementation.Run();
            while (_lock.exchange(true, std::memory_order_relaxed));
            _pendingJob = false;
            _lock.store(false, std::memory_order_relaxed);
        }

    private:
        bool _pendingJob;
        IMPLEMENTATION& _implementation;
        std::atomic<bool> _lock;
    };

}
}
