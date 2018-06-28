#include "Bluetooth.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(Bluetooth, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<BTStatus> > jsonResponseFactoryBTStatus(1);
    static Core::ProxyPoolType<Web::JSONBodyType<BTDeviceList> > jsonResponseFactoryBTDeviceList(1);
    static Core::ProxyPoolType<Web::JSONBodyType<BTDeviceList::BTDeviceInfo> > jsonResponseFactoryBTDeviceInfo(1);

    /* virtual */ const string Bluetooth::Initialize(PluginHost::IShell* service)
    {
        Config config;
        string message;

        ASSERT(_service == nullptr);

        _pid = 0;
        _service = service;
        _skipURL = _service->WebPrefix().length();
        config.FromString(_service->ConfigLine());

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        if (config.OutOfProcess.Value() == true)
            _bluetooth = _service->Instantiate<Exchange::IBluetooth>(3000, _T("BluetoothImplementation"), static_cast<uint32_t>(~0), _pid, _service->Locator());
        else
            _bluetooth = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IBluetooth>(Core::Library(), _T("BluetoothImplementation"), static_cast<uint32_t>(~0));

        if (_bluetooth == nullptr) {
            message = _T("Bluetooth could not be instantiated.");
            _service->Unregister(&_notification);
            _service = nullptr;
        }
        else
            _bluetooth->Configure(_service);

        return message;
    }

    /*virtual*/ void Bluetooth::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_bluetooth != nullptr);

        _service->Unregister(&_notification);

        if (_bluetooth->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

            ASSERT(_pid != 0);

            TRACE_L1("Bluetooth Plugin is not properly destructed. %d", _pid);

            RPC::IRemoteProcess* process(_service->RemoteProcess(_pid));

            // The process can disappear in the meantime...
            if (process != nullptr) {

                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                process->Terminate();
                process->Release();
            }
        }

        // Deinitialize what we initialized..
        _bluetooth = nullptr;
        _service = nullptr;
    }

    /* virtual */ string Bluetooth::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }

    /* virtual */ void Bluetooth::Inbound(WPEFramework::Web::Request& request)
    {
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length() - _skipURL)), false, '/');
        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        if ((request.Verb == Web::Request::HTTP_PUT) || (request.Verb == Web::Request::HTTP_POST)) {
            if ((index.IsValid() == true) && (index.Next() && index.IsValid())) {
                if ((index.Remainder() == _T("Pair")) || (index.Remainder() == _T("Connect")))
                   request.Body(jsonResponseFactoryBTDeviceInfo.Element());
            }
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> Bluetooth::Process(const WPEFramework::Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received Bluetooth request"))));

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        if (_bluetooth != nullptr) {
            if (request.Verb == Web::Request::HTTP_GET) {

                result = GetMethod(index);
            } else if (request.Verb == Web::Request::HTTP_PUT) {

                result = PutMethod(index, request);
            } else if (request.Verb == Web::Request::HTTP_POST) {

                result = PostMethod(index, request);
            } else if (request.Verb == Web::Request::HTTP_DELETE) {

                result = DeleteMethod(index);
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> Bluetooth::GetMethod(Core::TextSegmentIterator& index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                if (index.Remainder() == _T("DiscoveredDevices")) {

                    TRACE(Trace::Information, (string(__FUNCTION__)));
                    Core::ProxyType<Web::JSONBodyType<BTDeviceList> > response(jsonResponseFactoryBTDeviceList.Element());

                    std::string discoveredDevices = _bluetooth->DiscoveredDevices();
                    if (discoveredDevices.size() > 0) {
                        response->DeviceList.FromString(discoveredDevices);

                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Discovered devices.");
                        result->Body(response);
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Unable to display Discovered devices.");
                    }
                } else if (index.Remainder() == _T("PairedDevices")) {

                    TRACE(Trace::Information, (string(__FUNCTION__)));
                    Core::ProxyType<Web::JSONBodyType<BTDeviceList> > response(jsonResponseFactoryBTDeviceList.Element());

                    std::string pairedDevices = _bluetooth->PairedDevices();
                    if (pairedDevices.size() > 0) {
                        response->DeviceList.FromString(pairedDevices);

                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Paired devices.");
                        result->Body(response);
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Unable to display Paired devices.");
                    }
                }
            }
        } else {
            Core::ProxyType<Web::JSONBodyType<BTStatus> > response(jsonResponseFactoryBTStatus.Element());

            result->ErrorCode = Web::STATUS_OK;
            result->Message = _T("Current status.");

            response->Scanning = _bluetooth->IsScanning();
            response->Connected = _bluetooth->Connected();
            result->Body(response);
        }

        return result;
    }

    Core::ProxyType<Web::Response> Bluetooth::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported PUT request.");

        if (index.IsValid() == true) {
            if (index.Next()) {
                TRACE(Trace::Information, (string(__FUNCTION__)));

                if (index.Remainder() == _T("Scan")) {
                    if (_bluetooth->Scan()) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Scan started.");
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to start Scan.");
                    }
                } else if (index.Remainder() == _T("StopScan")) {
                    if (_bluetooth->StopScan()) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Scan stopped.");
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to stop Scan.");
                    }
                } else if ((index.Remainder() == _T("Pair")) && (request.HasBody())) {
                    Core::ProxyType<const BTDeviceList::BTDeviceInfo> deviceInfo (request.Body<const BTDeviceList::BTDeviceInfo>());
                    if (_bluetooth->Pair(deviceInfo->Address)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Paired device.");
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to Pair device.");
                    }
                } else if ((index.Remainder() == _T("Connect")) && (request.HasBody())) {
                    Core::ProxyType<const BTDeviceList::BTDeviceInfo> deviceInfo (request.Body<const BTDeviceList::BTDeviceInfo>());
                    if (_bluetooth->Connect(deviceInfo->Address)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Connected device.");
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to Connect device.");
                    }
                }
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> Bluetooth::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST requestservice.");

        return result;
    }

    Core::ProxyType<Web::Response> Bluetooth::DeleteMethod(Core::TextSegmentIterator& index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported DELETE request.");

        if (index.IsValid() == true) {
            if (index.Next()) {
                if (index.Remainder() == _T("Connect")) {
                    if (_bluetooth->Disconnect()) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Disconnected device.");
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to Disconnect device.");
                    }
                }
            }
        }

        return result;
    }

    void Bluetooth::Deactivated(RPC::IRemoteProcess* process)
    {
        if (process->Id() == _pid) {

            ASSERT(_service != nullptr);

            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }
}
} //namespace WPEFramework::Plugin
