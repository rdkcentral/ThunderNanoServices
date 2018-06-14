#include "Bluetooth.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(Bluetooth, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<BTDeviceList> > jsonResponseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<BTDeviceList> > jsonDataFactory(2);

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

        if (config.OutOfProcess.Value() == true) {
            _bluetooth = _service->Instantiate<Exchange::IPluginBluetooth>(3000, _T("BluetoothImplementation"), static_cast<uint32>(~0), _pid, _service->Locator());
        }
        else {
            _bluetooth = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IPluginBluetooth>(Core::Library(), _T("BluetoothImplementation"), static_cast<uint32>(~0));
        }

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
        if (request.Verb == Web::Request::HTTP_POST)
            request.Body(jsonDataFactory.Element());
    }

    /* virtual */ Core::ProxyType<Web::Response> Bluetooth::Process(const WPEFramework::Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received Bluetooth request"))));

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        if (_bluetooth != nullptr) {
            if ((request.Verb == Web::Request::HTTP_GET) && ((index.Next()) && (index.Next()))) {
                TRACE(Trace::Information, (string(__FUNCTION__)));

                if (index.Remainder() == _T("ShowDeviceList")) {
                    TRACE(Trace::Information, (string(__FUNCTION__)));
                    Core::ProxyType<Web::JSONBodyType<BTDeviceList> > response(jsonResponseFactory.Element());

                    std::string deviceInfoList = _bluetooth->ShowDeviceList();
                    if (deviceInfoList.size() > 0) {
                        response->DeviceInfoList.FromString(deviceInfoList);
                        result->ContentType = Web::MIMETypes::MIME_JSON;
                        result->Body(Core::proxy_cast<Web::IBody>(response));
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = "Unable to display DeviceId List";
                    }
                }
            } else if ((request.Verb == Web::Request::HTTP_POST) && (index.Next()) && (index.Next())) {
                TRACE(Trace::Information, (string(__FUNCTION__)));

                if (index.Remainder() == _T("StartScan")) {
                    if (!_bluetooth->StartScan()) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = "Unable to Start Scan";
                    }
                } else if (index.Remainder() == _T("StopScan")) {
                    if (!_bluetooth->StopScan()) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = "Unable to Stop Scan";
                    }
                } else if ((index.Remainder() == _T("PairDevice")) && (request.HasBody())) {
                    std::string deviceId = request.Body<const BTDeviceList>()->DeviceId.Value();
                    if (!_bluetooth->Pair(deviceId)) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = "Unable to Pair Device";
                    }
                } else if ((index.Remainder() == _T("ConnectDevice")) && (request.HasBody())) {
                    std::string deviceId = request.Body<const BTDeviceList>()->DeviceId.Value();
                    if (!_bluetooth->Connect(deviceId)) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = "Unable to Connect Device";
                    }
                } else if ((index.Remainder() == _T("DisconnectDevice")) && (request.HasBody())) {
                    std::string deviceId = request.Body<const BTDeviceList>()->DeviceId.Value();
                    if (!_bluetooth->Disconnect(deviceId)) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = "Unable to Disconnect Device";
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
