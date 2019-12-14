#include "OCDM.h"
#include <ocdm/open_cdm.h>
#include <interfaces/IDRM.h>

namespace WPEFramework {

namespace OCDM {

    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
    {
        class MemoryObserverImpl : public Exchange::IMemory {
        private:
            MemoryObserverImpl();
            MemoryObserverImpl(const MemoryObserverImpl&);
            MemoryObserverImpl& operator=(const MemoryObserverImpl&);

        public:
            MemoryObserverImpl(const RPC::IRemoteConnection* connection)
                : _main(connection  == 0 ? Core::ProcessInfo().Id() : connection->RemoteId())
            {
            }
            ~MemoryObserverImpl()
            {
            }

        public:
            virtual void Observe(const uint32_t pid)
            {
                if (pid == 0) {
                    _observable = false;
                } else {
                    _observable = false;
                    _main = Core::ProcessInfo(pid);
                }
            }
            virtual uint64_t Resident() const
            {
                return (_observable == false ? 0 : _main.Resident());
            }
            virtual uint64_t Allocated() const
            {
                return (_observable == false ? 0 : _main.Allocated());
            }
            virtual uint64_t Shared() const
            {
                return (_observable == false ? 0 : _main.Shared());
            }
            virtual uint8_t Processes() const
            {
                return (IsOperational() ? 1 : 0);
            }
            virtual const bool IsOperational() const
            {
                return (_observable == false) || (_main.IsActive());
            }

            BEGIN_INTERFACE_MAP(MemoryObserverImpl)
            INTERFACE_ENTRY(Exchange::IMemory)
            END_INTERFACE_MAP

        private:
            Core::ProcessInfo _main;
            bool _observable;
        };

        Exchange::IMemory* result = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);

        return (result);
    }
}

namespace Plugin {

    SERVICE_REGISTRATION(OCDM, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<OCDM::Data>> jsonDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<OCDM::Data::System>> jsonSystemFactory(1);

    /* virtual */ const string OCDM::Initialize(PluginHost::IShell* service)
    {
		#ifdef __WINDOWS__
        ForceLinkingOfOpenCDM();
		#endif

        string message;

        ASSERT(_service == nullptr);
        ASSERT(_memory == nullptr);
        ASSERT(_opencdmi == nullptr);

        _connectionId = 0;
        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        _opencdmi = _service->Root<Exchange::IContentDecryption>(_connectionId, Core::infinite, _T("OCDMImplementation"));

        if (_opencdmi == nullptr) {
            message = _T("OCDM could not be instantiated.");
            _service->Unregister(&_notification);
            _service = nullptr;
        } else {
            _opencdmi->Initialize(_service);

            if ((_connectionId != 0) && (_service->RemoteConnection(_connectionId) == nullptr)){
                message = _T("OCDM crashed at initialize!");
                _opencdmi = nullptr;
                _service->Unregister(&_notification);
                _service = nullptr;
            } else {
                RPC::IRemoteConnection* connection = _service->RemoteConnection(_connectionId);

                ASSERT(connection != nullptr);

                if(connection != nullptr) {
                    _memory = WPEFramework::OCDM::MemoryObserver(connection);
                    connection->Release();
                }
                
                ASSERT(_memory != nullptr);
            }
        }

        return message;
    }

    /*virtual*/ void OCDM::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_memory != nullptr);
        ASSERT(_opencdmi != nullptr);

        _service->Unregister(&_notification);
        _memory->Release();

        _opencdmi->Deinitialize(service);

        if (_opencdmi->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

            ASSERT(_connectionId != 0);

            TRACE_L1("OCDM Plugin is not properly destructed. %d", _connectionId);

            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            // The process can disappear in the meantime...
            if (connection != nullptr) {

                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                connection->Terminate();
                connection->Release();
            }
        }

        PluginHost::ISubSystem* subSystem = service->SubSystems();

        ASSERT(subSystem != nullptr);

        if (subSystem != nullptr) {
            ASSERT(subSystem->IsActive(PluginHost::ISubSystem::DECRYPTION) == true);
            subSystem->Set(PluginHost::ISubSystem::NOT_DECRYPTION, nullptr);
            subSystem->Release();
        }

        // Deinitialize what we initialized..
        _memory = nullptr;
        _opencdmi = nullptr;
        _service = nullptr;
    }

    /* virtual */ string OCDM::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }

    /* virtual */ void OCDM::Inbound(Web::Request& request)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> OCDM::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received OCDM request"))));

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length() - _skipURL)), false, '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");

        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        if (request.Verb == Web::Request::HTTP_GET) {

            if (index.Next() == false) {
                Core::ProxyType<Web::JSONBodyType<Data>> data(jsonDataFactory.Element());

                RPC::IStringIterator* systems(_opencdmi->Systems());

                if (systems == nullptr) {
                    TRACE_L1("Could not load the Systems. %d", __LINE__);
                } else {
                    string element;
                    while (systems->Next(element) == true) {
                        RPC::IStringIterator* index(_opencdmi->Designators(element));
                        if (index == nullptr) {
                            TRACE_L1("Could not load the Designators. %d", __LINE__);
                        } else {
                            data->Systems.Add(Data::System(systems->Current(), index));
                            index->Release();
                        }
                    }
                    systems->Release();
                }
                result->ErrorCode = Web::STATUS_OK;
                result->Message = _T("Available Systems.");
                result->Body(data);
            } else {
                if (index.Current().Text() == _T("Sessions")) {

                    // Core::ProxyType<Web::JSONBodyType<WifiControl::NetworkList> > networks (jsonResponseFactoryNetworkList.Element());

                    // virtual RPC::IStringIterator* Sessions(const string& keySystem);
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Active Sessions.");
                    // result->Body(networks);

                } else if (index.Current().Text() == _T("Designators")) {
                    if (index.Next() == true) {

                        Core::ProxyType<Web::JSONBodyType<OCDM::Data::System>> data(jsonSystemFactory.Element());
                        RPC::IStringIterator* entries(_opencdmi->Designators(index.Current().Text()));

                        if (entries == nullptr) {
                            TRACE_L1("Could not load the Designators. %d", __LINE__);
                        } else {
                            data->Name = index.Current().Text();
                            data->Load(entries);
                            entries->Release();
                        }

                        result->Body(data);
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Available Systems.");
                    }
                }
            }
        }

        return result;
    }

    void OCDM::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {

            ASSERT(_service != nullptr);

            Core::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }
}
} //namespace WPEFramework::Plugin
