#pragma once

#include "Module.h"
#include <unordered_map>

namespace Thunder {
namespace Plugin {
    class JsonRpcMuxer 
        : public PluginHost::IPlugin
        , public PluginHost::JSONRPC {
    private:
        class Config : public Core::JSON::Container {
        public:
            Config(Config&&) = delete;
            Config(const Config&) = delete;
            Config& operator=(Config&&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , TimeOut(Core::infinite)
                , MaxBatchSize(10)
                , MaxBatches(5)
            {
                Add(_T("timeout"), &TimeOut);
                Add(_T("maxbatchsize"), &MaxBatchSize);
                Add(_T("maxbatches"), &MaxBatches);
            }
            ~Config() override = default;

        public:
            Core::JSON::DecUInt32 TimeOut;
            Core::JSON::DecUInt8 MaxBatchSize;
            Core::JSON::DecUInt8 MaxBatches;
        };

        class Batch {
        private:
            enum state : uint8_t {
                IN_PROGRESS = 0x01,
                COMPLETED   = 0x02,
                ABORTED     = 0x03
            };

            class Job {
            public:
                Job() = delete;
                Job(Job&&) = delete;
                Job(const Job&) = delete;
                Job& operator= (Job&&) = delete;
                Job& operator= (const Job&) = delete;

                Job(Batch& parent, const uint8_t index) 
                   : _parent(parent)
                   , _index(index) {
                }
                ~Job() = default;

            public:
                void Dispatch() {
                    _parent.Dispatch(_index);
                }

            private:
                Batch&  _parent;
                uint8_t _index;
            };

            class Request {
            public:
                Request() = delete;
                Request(Request&&) = delete;
                Request(const Request&) = delete;
                Request& operator= (Request&&) = delete;
                Request& operator= (const Request&) = delete;

                Request (const Core::JSONRPC::Message& message) 
                    : _id(message)
                    , _designator(message)
                    , _data(message)
                    , _error(0)
                    , _completed(false) {
                }
                ~Request() = default;

            public:
                inline bool IsCompleted() const {
                    return(_completed);
                }
                inline uint32_t Id() const {
                    return (_id);
                }
                inline const string& Designator() const {
                    return(_designator);
                }
                inline const string& Parameters() const {
                    return (_data);
                }
                inline void Set(const Core::hresult errorCode, const string& output) {
                    _completed = true;
                    if (errorCode == Core::ERROR_NONE) {
                        _data = output;
                    }
                    else {
                        _error = error
                        _data = output;
                    }
                }

            private:
                const uint32_t _id;
                const string _designator;
                string _data; // parameters, on input, error response or result on output
                sint32 _error;
                bool _completed;
            };

        public:
            Batch(Batch&&) = delete;
            Batch(const Batch&) = delete;
            Batch& operator=(const Batch&) = delete;
            Batch& operator=(Batch&&) = delete;

            Batch(
                JsonRpcMuxer&  parent,
                const uint8_t  parallel,
                const uint32_t channelId,
                const uint32_t responseId,
                const string&  token,
                const uint32_t timeoutMs,
                const Core::JSON::ArrayType<Core::JSONRPC::Message>& requests)
                : _adminLock()
                , _parent(parent)
                , _state(state::IN_PROGRESS)
                , _requests()
                , _jobs()
                , _index(0)
                , _channelId(channelId)
                , _responseId(responseId)
                , _startTime()
            {
                ASSERT (parallel >= 1);

                // Move all requests into the batch
                _requests.reserve(requests.count());
                Core::JSON::ArrayType<const Core::JSONRPC::Message>::ConstIterator index(requests.Elements());

                while (index.Next() == true) {
                    _request.emplace_back(index.Current());
                }

                // Create the number of jobs that are allowed to run in parallel..
                uint8_t jobs(static_cast<uint8_t>(std::min(static_cast<uint32_t>(parallel), static_cast<uint32_t>(_requests.size()))));
                _jobs.reserve(jobs);
                for (uint8_t position = 0; position < jobs; position++) {
                    _jobs.emplace_back(Core::ProxyType<Job>::Create(*this, position));
                }
            }
            ~Batch() = default;

        public:
            inline const uint64_t& StartTime() const {
                return (_startTime);
            }
            inline bool IsCompleted() const {
                return (_state == state::COMPLETED);
            }
            void Activate() {
                _adminLock.Lock();
                if (state == state::IN_PROGRESS) {
                    _startTime = Core::Time::Now();
                    for (auto& element : _jobs) {
                        Core::ProxyType<Core::IDispatch> job(element.Submit());

                        if (job.IsValid()) {
                            IWorkerPool::Instance().Submit(job);
                        }
                    }
                }
                _adminLock.Unlock();
            }
            void Abort()
            {
                _adminLock.Lock();
                if (state == state::IN_PROGRESS) {
                    _state = state::ABORTED;
                    for (auto& element : _jobs) {
                        Core::ProxyType<IDispatch> job(element.Revoke());

                        if (job.IsValid() == true) {
                            if (Core::IWorkerPool::Instance().Revoke(job, 0) != Core::ERROR_TIMEDOUT) {
                                element.Revoked();
                            }
                        }
                    }
                }
                _adminLock.Unlock();
            }
            uint32_t WaitForIdle(const uint32_t time) const
            {
                uint32_t left(time);
                uint8_t index(0);
                uint32_t result(Core::ERROR_TIMEDOUT);

                // No need to lock, the std::vector of jobs is fixed from construction to destruction :-)
                while ((result == Core::ERROR_TIMEDOUT) && (left > 0)) {
                    if (_jobs[index].IsIdle() == false) {
                        Core::ProxyType<IDispatch> job(_jobs[index].Revoke());
                        if (job.IsValid() == false) {
                            ASSERT(_jobs[index].IsIdle());
                            ++index;
                        }
                        else {
                            if (Core::IWorkerPool::Instance().Revoke(job, TimeSlot) == Core::ERROR_TIMEDOUT) {
                                left = (time == Core::infinite ? left : left - std::min(left, TimeSlot));
                            }
                            else {
                                element.Revoked();
                                ++index;
                            }
                        }
                    }
                    else if (index < _jobs.size()) {
                        ++index;
                    }
                    else {
                        result = Core::ERROR_NONE;
                    }
                }

                return (result);
            }

        private:
            void Dispatch(const uint8_t index) {
                ASSERT(index < _jobs.size());

                // Oke, lets pick a request to handle, do it in a locked fashion so the
                // administration is always correct.
                _adminLock.Lock();

                if ((_state == state::IN_PROGRESS) && (_index < _requests.size())) {
                    Request& msg(_request[_index]);
                    _index++;
                    _adminLock.Unlock();

                    string output;
                    Core::hresult result = _parent.Invoke (_channelId, msg.Id(), msg.Designator(), msg.Parameters(), output);
                    msg.Set(result, output);

                    _adminLock.Lock();
                    if (_state == state::IN_PROGRESS) {
                        if (_index < _requests.size()) {
                            Core::ProxyType<Core::IDispatch> job(_jobs[index].Submit());

                            if (job.IsValid()) {
                                IWorkerPool::Instance().Submit(job);
                            }
                        }
                        else {
                            uint32_t position(0);
                            while ((position < _requests.size()) && (_requests[position].IsCompleted() == true)) {
                                ++position;
                            }
                            if (position == _requests.size()) {
                                _state = state::COMPLETED;
                                _parent.Completed();
                            }
                        }
                    }
                }

                _adminLock.Unlock();
            }

        private:
            mutable Core::CriticalSection _adminLock;
            JsonRpcMuxer& _parent;
            state _state;
            std::vector<Request> _requests;
            std::vector< Core::ThreadPool::JobType<Job> > _jobs;
            uint32_t _index;
            uint32_t _channelId;
            uint32_t _responseId;
            uint64_t _startTime;
        };

    public:
        JsonRpcMuxer(JsonRpcMuxer&&) = delete;
        JsonRpcMuxer(const JsonRpcMuxer&) = delete;
        JsonRpcMuxer& operator=(JsonRpcMuxer&&) = delete;
        JsonRpcMuxer& operator=(const JsonRpcMuxer&) = delete;

        JsonRpcMuxer()
            : _adminLock()
            , _service(nullptr)
            , _dispatch(nullptr)
            , _activeWebSocket(0)
            , _batchCounter(0)
            , _maxBatchSize(10)
            , _maxBatches(5)
            , _timeout(Core::infinite)
            , _shuttingDown(false)
            , _activeBatches()
            , _activeBatchCount(0)
            , _batchCompletionEvent(false, true)
            , _job(*this) {
        }
        ~JsonRpcMuxer() override = default;

    public:
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        Core::hresult Invoke(const uint32_t channelId, const uint32_t id,
            const string& designator, const string& parameters, string& response) override;

        BEGIN_INTERFACE_MAP(JsonRpcMuxer)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    private:
        void Dispatch() {
            _adminLock.Lock();

            std::list<Batch>::iterator index(_batches.begin());

            while (index != _batches.end()) {
                if (index->IsCompleted() == false) {
                    index++;
                }
                else {
                    index = _batches.erase(index);
                }
            }

            _adminLock.Unlock();
        }
        void Completed() {
            _job.Submit();
        }

    private:
        friend class Batch;

        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        PluginHost::IDispatcher* _dispatch;
        uint32_t _activeWebSocket;
        uint8_t _maxBatchSize;
        uint8_t _maxBatches;
        uint32_t _timeout;
        uint32_t _batchCounter;
        uint8_t _activeBatchCount;

        std::list<Batch> _batches;
        Core::IWorkerPool::JobType<JsonRpcMuxer&> _job;
    };
}
}
