/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

#include "JsonRpcMuxer.h"

namespace Thunder {
namespace Plugin {
    namespace {
        static Metadata<JsonRpcMuxer> metadata(
            PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_PATCH,
            {}, {}, {});
    }

    class JsonRpcMuxer::Processor {
    private:
        class Batch {
        private:
            enum state : uint8_t {
                IN_PROGRESS = 0x01,
                COMPLETED = 0x02,
                ABORTED = 0x03
            };

            class Response : public Core::JSON::Container {
            public:
                ~Response() override = default;

                Response()
                    : Core::JSON::Container()
                    , Id(~0)
                    , Result(false)
                    , Error()
                {
                    Add(_T("id"), &Id);
                    Add(_T("result"), &Result);
                    Add(_T("error"), &Error);
                    Clear();
                }

                Response(const Response& copy)
                    : Core::JSON::Container()
                    , Id(copy.Id)
                    , Result(copy.Result)
                    , Error(copy.Error)
                {
                    Add(_T("id"), &Id);
                    Add(_T("result"), &Result);
                    Add(_T("error"), &Error);
                }

                Response(Response&& move) noexcept
                    : Core::JSON::Container()
                    , Id(std::move(move.Id))
                    , Result(std::move(move.Result))
                    , Error(std::move(move.Error))
                {
                    Add(_T("id"), &Id);
                    Add(_T("result"), &Result);
                    Add(_T("error"), &Error);
                }

                Core::JSON::DecUInt32 Id;
                Core::JSON::String Result;
                Core::JSONRPC::Message::Info Error;
            };

            class Request {
            public:
                Request() = delete;
                Request(const Request&) = delete;
                Request& operator=(Request&&) = delete;
                Request& operator=(const Request&) = delete;

                Request(const Core::JSONRPC::Message& message)
                    : _id(message.Id.Value())
                    , _designator(message.Designator.Value())
                    , _data(message.Parameters.Value())
                    , _errorCode(Core::ERROR_GENERAL)
                    , _completed(false)
                {
                }
                ~Request() = default;

                Request(Request&& other) noexcept
                    : _id(other._id)
                    , _designator(other._designator)
                    , _data(std::move(other._data))
                    , _errorCode(other._errorCode)
                    , _completed(other._completed)
                {
                }

            public:
                inline bool IsCompleted() const
                {
                    return (_completed);
                }
                inline uint32_t Id() const
                {
                    return (_id);
                }
                inline const string& Designator() const
                {
                    return (_designator);
                }
                inline const string& Parameters() const
                {
                    return (_data);
                }
                inline void Set(const Core::hresult errorCode, const string& output)
                {
                    _completed = true;
                    if (errorCode == Core::ERROR_NONE) {
                        _data = output;
                    } else {
                        _errorCode = errorCode;
                        _data = output;
                    }
                }
                inline Core::hresult ErrorCode() const
                {
                    return (_errorCode);
                }

            private:
                const uint32_t _id;
                const string _designator;
                string _data;
                Core::hresult _errorCode;
                bool _completed;
            };

        public:
            class Job : public Core::IDispatch {
            public:
                Job() = delete;
                Job(Job&&) = delete;
                Job(const Job&) = delete;
                Job& operator=(const Job&) = delete;
                Job& operator=(Job&&) = delete;

                Job(Batch& parent, Request& request)
                    : _parent(parent)
                    , _context(request)
                {
                    TRACE(Trace::Information, (_T("Constructing job %p"), this));
                }

                virtual ~Job()
                {
                    TRACE(Trace::Information, (_T("Destructing job %p"), this));
                }

                void Dispatch() override
                {
                    TRACE(Trace::Information, (_T("Dispatch job %p"), this));
                    _parent.Process(this);
                    TRACE(Trace::Information, (_T("Dispatch job %p Done"), this));
                }

                Request& Context()
                {
                    return _context;
                }

            private:
                Batch& _parent;
                Request& _context;
            };

            Batch() = delete;
            Batch(Batch&&) = delete;
            Batch(const Batch&) = delete;
            Batch& operator=(const Batch&) = delete;
            Batch& operator=(Batch&&) = delete;

            Batch(Processor& parent, bool concurrent,
                const Core::JSON::ArrayType<Core::JSONRPC::Message>& requests,
                uint32_t channelId, uint32_t responseId, const string& token)
                : _lock()
                , _parent(parent)
                , _channelId(channelId)
                , _responseId(responseId)
                , _token(token)
                , _concurrent(concurrent)
                , _requests()
                , _current(0)
                , _completed(0)
                , _state(IN_PROGRESS)
            {
                Core::JSON::ArrayType<Core::JSONRPC::Message>::ConstIterator index(requests.Elements());
                _requests.reserve(requests.Elements().Count());

                while (index.Next() == true) {
                    _requests.emplace_back(index.Current());
                }

                TRACE(Trace::Information, (_T("Created batch[responseId] with %d requests"), _requests.size()));
            }

            ~Batch()
            {
                Abort();
                _requests.clear();

                TRACE(Trace::Information, (_T("Destructing batch[responseId]")));
            }

            uint32_t ChannelId() const
            {
                return _channelId;
            }

            uint32_t ResponseId() const
            {
                return _responseId;
            }

            bool Sequential() const
            {
                return _concurrent == false;
            }

            bool IsFinished() const
            {
                return (_state == COMPLETED) || (_state == ABORTED);
            }

            void Abort() const
            {
                _lock.Lock();
                _state = ABORTED;
                _lock.Unlock();
            }

            string Serialize() const
            {
                Core::JSON::ArrayType<Response> responses;

                _lock.Lock();

                for (const auto& request : _requests) {
                    Response resp;

                    resp.Id = request.Id();
                    if (request.IsCompleted()) {
                        if (request.ErrorCode() == Core::ERROR_NONE) {
                            resp.Result = request.Parameters();
                        } else {
                            resp.Error.Code = request.ErrorCode();
                            resp.Error.Text = request.Parameters();
                        }
                    } else {
                        resp.Error.Code = Core::ERROR_GENERAL;
                        resp.Error.Text = _T("Request not completed");
                    }

                    responses.Add(resp);
                }

                _lock.Unlock();

                string result;
                responses.ToString(result);

                return result;
            }

            Core::ProxyType<Job> GetJob()
            {
                Core::ProxyType<Job> job;

                if (_current < _requests.size()) {
                    job = Core::ProxyType<Job>::Create(*this, _requests[_current]);
                    _current++;
                    TRACE(Trace::Information, (_T("Batch[%d] Provided job %d/%d"), _responseId, _current, _requests.size()));
                }

                return job;
            }

        private:
            void Process(Job* job)
            {
                ASSERT(job != nullptr);

                string response;
                Core::hresult result = Core::ERROR_NONE;
                Request& request = job->Context();

                _lock.Lock();

                if ((_state != ABORTED) && (_state != COMPLETED)) {
                    _lock.Unlock();

                    result = _parent.Invoke(_channelId, request.Id(), _token, request.Designator(), request.Parameters(), response);
                    request.Set(result, response);

                    _lock.Lock();

                    ++_completed;

                    TRACE(Trace::Information, (_T("Batch[%d] completed request[%d] (%d/%d)"), _responseId, request.Id(), _completed, _requests.size()));

                    if (_completed == _requests.size()) {
                        _state = COMPLETED;
                    }
                }

                _lock.Unlock();

                _parent.Checkout(job);
            }

        private:
            mutable Core::CriticalSection _lock;
            Processor& _parent;
            const uint32_t _channelId;
            const uint32_t _responseId;
            const string _token;
            const bool _concurrent;
            mutable std::vector<Request> _requests;
            uint16_t _current;
            uint16_t _completed;
            mutable state _state;
        };

    public:
        Processor() = delete;
        Processor(Processor&&) = delete;
        Processor(const Processor&) = delete;
        Processor& operator=(const Processor&) = delete;
        Processor& operator=(Processor&&) = delete;

        explicit Processor(PluginHost::IShell* service, const uint8_t maxConcurrentJobs)
            : _lock()
            , _service(service)
            , _dispatch(nullptr)
            , _maxConcurrentJobs(maxConcurrentJobs)
            , _shuttingDown(false)
            , _batches()
            , _activeJobs()
        {
            ASSERT(_service != nullptr);

            _service->AddRef();

            _dispatch = _service->QueryInterfaceByCallsign<PluginHost::IDispatcher>("Controller");

            ASSERT(_dispatch != nullptr);

            if (_dispatch != nullptr) {
                _dispatch->AddRef();
            }
        }

        ~Processor()
        {
            Stop();

            if (_dispatch != nullptr) {
                _dispatch->Release();
                _dispatch = nullptr;
            }

            if (_service != nullptr) {
                _service->Release();
                _service = nullptr;
            }
        }

        void Stop()
        {
            std::vector<Core::ProxyType<Batch::Job>> jobsToRevoke;

            _lock.Lock();

            _shuttingDown = true;

            for (Batch* batch : _batches) {
                batch->Abort();
            }

            jobsToRevoke = std::move(_activeJobs);

            _lock.Unlock();

            for (auto& job : jobsToRevoke) {
                Core::IWorkerPool::Instance().Revoke(
                    Core::ProxyType<Core::IDispatch>(job),
                    Core::infinite);
            }

            _lock.Lock();

            for (Batch* batch : _batches) {
                delete batch;
            }
            _batches.clear();

            _lock.Unlock();
        }

        Core::hresult Submit(const Core::JSON::ArrayType<Core::JSONRPC::Message>& messages, uint32_t channelId, uint32_t responseId, const string& token, bool concurrent)
        {
            _lock.Lock();

            if (_shuttingDown) {
                _lock.Unlock();
                TRACE(Trace::Warning, (_T("Rejected batch submission during shutdown")));
                return Core::ERROR_ABORTED;
            }

            Batch* batch = new Batch(*this, concurrent, messages, channelId, responseId, token);

            _batches.push_back(batch);

            _lock.Unlock();

            Process();

            return ~0;
        }

    private:
        void Checkout(Batch::Job* job)
        {
            ASSERT(job != nullptr);

            _lock.Lock();

            if (!_shuttingDown) {
                for (auto it = _activeJobs.begin(); it != _activeJobs.end();) {
                    if ((*it).operator->() == job) {
                        _activeJobs.erase(it);
                        break;
                    } else {
                        ++it;
                    }
                }
            }

            _lock.Unlock();

            TRACE(Trace::Information, (_T("Checkout job %p, active %d"), job, _activeJobs.size()));

            if (!_shuttingDown) {
                Process();
            }
        }

        uint32_t Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response)
        {
            if (_dispatch != nullptr) {
                return _dispatch->Invoke(channelId, id, token, method, parameters, response);
            } else {
                response = "IDispatcher not available";
                return Core::ERROR_UNAVAILABLE;
            }
        }

        void Process()
        {
            TRACE(Trace::Information, (_T("Start processing....")));

            _lock.Lock();

            if (_shuttingDown == false) {
                // First, remove finished batches
                for (auto it = _batches.begin(); it != _batches.end();) {
                    if ((*it)->IsFinished()) {

                        Batch* batch = (*it);

                        it = _batches.erase(it);
                        _lock.Unlock();

                        uint32_t answer = Answer(*batch);

                        if (answer != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Batch completion failed %d"), answer));
                        }

                        delete batch;

                        _lock.Lock();
                    } else {
                        ++it;
                    }
                }

                // Then schedule new jobs
                while ((_activeJobs.size() < _maxConcurrentJobs)) {
                    bool scheduledJob = false;

                    for (Batch* batch : _batches) {
                        if (batch->IsFinished()) {
                            continue;
                        }

                        // schedule as many jobs for this batch as allowed and as slots permit
                        while ((_activeJobs.size() < _maxConcurrentJobs)) {
                            Core::ProxyType<Batch::Job> job = batch->GetJob();

                            if (job.IsValid() == true) {
                                _activeJobs.emplace_back(job);
                                Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(job));
                                TRACE(Trace::Information, (_T("Submitted job %p..."), job.operator->()));
                                scheduledJob = true;

                                // if batch is sequential, only schedule one job for this batch
                                if (batch->Sequential() == true) {
                                    break;
                                }
                            } else {
                                break; // No more jobs from this batch
                            }
                        }

                        if (_activeJobs.size() >= _maxConcurrentJobs) {
                            break;
                        }
                    }

                    if (!scheduledJob) {
                        break;
                    }
                }
            }

            _lock.Unlock();
        }

        uint32_t Answer(const Batch& batch)
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_service) {
                Core::ProxyType<Core::JSONRPC::Message> response(PluginHost::IFactories::Instance().JSONRPC());

                response->Id = batch.ResponseId();
                response->Result = batch.Serialize();

                result = _service->Submit(batch.ChannelId(), Core::ProxyType<Core::JSON::IElement>(response));
            }

            return result;
        }

    private:
        mutable Core::CriticalSection _lock;
        PluginHost::IShell* _service;
        PluginHost::IDispatcher* _dispatch;
        const uint8_t _maxConcurrentJobs;
        bool _shuttingDown;
        std::vector<Batch*> _batches;
        std::vector<Core::ProxyType<Batch::Job>> _activeJobs;
    };

    JsonRpcMuxer::JsonRpcMuxer()
        : _processor(nullptr)
        , _maxBatchSize(Config::DefaultMaxBatchSize)
    {
    }

    JsonRpcMuxer::~JsonRpcMuxer()
    {
    }

    const string JsonRpcMuxer::Initialize(PluginHost::IShell* service)
    {
        string result;

        ASSERT(service != nullptr);

        Config config;
        config.FromString(service->ConfigLine());

        const Core::WorkerPool::Metadata metaData = Core::WorkerPool::Instance().Snapshot();

        uint8_t maxConcurrentJobs;

        if (config.MaxConcurrentJobs.IsSet()) {
            maxConcurrentJobs = std::min<uint8_t>(config.MaxConcurrentJobs.Value(), metaData.Slots);
        } else {
            maxConcurrentJobs = std::max<uint8_t>(1, static_cast<uint8_t>(metaData.Slots / 2));
        }

        if (config.MaxBatchSize.IsSet() == true) {
            _maxBatchSize = config.MaxBatchSize.Value();
        }

        _processor = new Processor(service, maxConcurrentJobs);

        TRACE(Trace::Information, (_T("Initialized successfully, Max concurrent jobs: %d, Max batch size: %d"), maxConcurrentJobs, _maxBatchSize));
        return result;
    }

    void JsonRpcMuxer::Deinitialize(PluginHost::IShell* /*service*/)
    {
        TRACE(Trace::Information, (_T("Deinitializing...")));

        if (_processor != nullptr) {
            delete _processor;
            _processor = nullptr;
        }

        TRACE(Trace::Information, (_T("Deinitialized")));
    }

    string JsonRpcMuxer::Information() const
    {
        return string();
    }

    Core::hresult JsonRpcMuxer::Invoke(const uint32_t channelId, const uint32_t id, const string& token,
        const string& designator, const string& parameters, string& response)
    {
        const string method = Core::JSONRPC::Message::Method(designator);

        Core::hresult result(Core::ERROR_NONE);

        if (_processor == nullptr) {
            result = Core::ERROR_UNAVAILABLE;
            response = _T("Processor is offline");
            TRACE(Trace::Error, (_T("Invoke failed: Processor unavailable")));
        } else if (method != _T("parallel") && method != _T("sequential")) {
            result = Core::ERROR_UNKNOWN_METHOD;
            response = _T("Invalid method, must be 'parallel' or 'sequential'");
            TRACE(Trace::Warning, (_T("Invalid method: %s"), method.c_str()));
        } else {
            Core::JSON::ArrayType<Core::JSONRPC::Message> messages;
            Core::OptionalType<Core::JSON::Error> parseError;

            if (!messages.FromString(parameters, parseError) || parseError.IsSet()) {
                result = Core::ERROR_PARSE_FAILURE;
                response = parseError.IsSet() ? parseError.Value().Message() : _T("Failed to parse message array");
                TRACE(Trace::Error, (_T("Parse failure: %s"), response.c_str()));
            } else if (messages.Length() == 0) {
                result = Core::ERROR_BAD_REQUEST;
                response = _T("Empty message array");
                TRACE(Trace::Warning, (_T("Empty message array received")));
            } else if (messages.Length() > _maxBatchSize) {
                result = Core::ERROR_BAD_REQUEST;
                response = _T("Batch size (") + Core::NumberType<uint32_t>(messages.Length()).Text() + _T(") exceeds maximum allowed (") + Core::NumberType<uint32_t>(_maxBatchSize).Text() + _T(")");
                TRACE(Trace::Warning, (_T("Batch size %u exceeds maximum %u"), messages.Length(), _maxBatchSize));
            } else {
                result = _processor->Submit(messages, channelId, id, token, (method == _T("parallel")));
            }
        }

        return result;
    }
}
}