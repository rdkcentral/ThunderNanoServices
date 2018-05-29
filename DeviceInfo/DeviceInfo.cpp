#include "DeviceInfo.h"

namespace WPEFramework
{
    namespace Plugin {

        SERVICE_REGISTRATION(DeviceInfo, 1, 0);

        static Core::ProxyPoolType<Web::Response> responseFactory(4);
        static Core::ProxyPoolType<Web::JSONBodyType<DeviceInfo::Data> > jsonResponseFactory(4);

        /* virtual */ const string DeviceInfo::Initialize(PluginHost::IShell* service)
        {
            ASSERT (_service == nullptr);
            ASSERT (service != nullptr);

            ASSERT(_subSystem == nullptr);

            Config config;
            config.FromString(service->ConfigLine());
            _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
            _subSystem = service->SubSystems();
            _service = service;
            _deviceId = GetDeviceId();
            _systemId = Core::SystemInfo::Instance().Id(Core::SystemInfo::Instance().RawDeviceId(), ~0);

            ASSERT(_subSystem != nullptr);

            // On success return empty, to indicate there is no error text.

            return (_subSystem != nullptr) ?  EMPTY_STRING : _T("Could not retrieve System Information.") ;
        }

        /* virtual */ void DeviceInfo::Deinitialize(PluginHost::IShell* service)
        {
            ASSERT (_service == service);

            if (_subSystem!= nullptr) {
                _subSystem->Release();
                _subSystem = nullptr;
            }

            _service = nullptr;
        }

        /* virtual */ string DeviceInfo::Information() const
        {
            // No additional info to report.
            return (string());
        }

        /* virtual */ void DeviceInfo::Inbound(Web::Request& /* request */)
        {
        }

        /* virtual */ Core::ProxyType<Web::Response> DeviceInfo::Process(const Web::Request& request)
        {
            ASSERT(_skipURL <= request.Path.length());

            Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

            // By default, we assume everything works..
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";

            // <GET> - currently, only the GET command is supported, returning system info
            if (request.Verb == Web::Request::HTTP_GET) {

                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());

                Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

                // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
                index.Next();

                if (index.Next() == false) {
                    AddressInfo(response->Addresses);
                    SysInfo(response->SystemInfo);
                    SocketPortInfo(response->Sockets);
                }
                else if (index.Current() == "Adresses") {
                    AddressInfo(response->Addresses);
                }
                else if (index.Current() == "System") {
                    SysInfo(response->SystemInfo);
                }
                else if (index.Current() == "Sockets") {
                    SocketPortInfo(response->Sockets);
                }
                // TODO RB: I guess we should do something here to return other info (e.g. time) as well.

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }
            else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Unsupported request for the [DeviceInfo] service.");
            }

            return result;
        }

        void DeviceInfo::SysInfo(Data::SysInfo& systemInfo)
        {
            Core::SystemInfo& singleton(Core::SystemInfo::Instance());

            if (_deviceId.empty() == true)
            {
                _deviceId = GetDeviceId();
            }
            if (_deviceId.empty() == false)
            {
                systemInfo.DeviceId = _deviceId;
            }

            systemInfo.Time = Core::Time::Now().ToRFC1123(true);
            systemInfo.Version = _service->Version() + _T("#") + _subSystem->BuildTreeHash();
            systemInfo.Uptime = singleton.GetUpTime();
            systemInfo.FreeRam = singleton.GetFreeRam();
            systemInfo.TotalRam = singleton.GetTotalRam();
            systemInfo.Hostname = singleton.GetHostName();
            systemInfo.CpuLoad = Core::NumberType<uint32_t>(static_cast<uint32_t>(singleton.GetCpuLoad())).Text();
            systemInfo.TotalGpuRam = singleton.GetTotalGpuRam();
            systemInfo.FreeGpuRam = singleton.GetFreeGpuRam();
            systemInfo.SerialNumber = _systemId;
        }

        void DeviceInfo::AddressInfo(Core::JSON::ArrayType<Data::AddressInfo>& addressInfo)
        {
            // Get the point of entry on WPEFramework..
            Core::AdapterIterator interfaces;

            while (interfaces.Next() == true) {

                Data::AddressInfo newElement;
                newElement.Name = interfaces.Name();
                newElement.MACAddress = interfaces.MACAddress(':');
                Data::AddressInfo& element(addressInfo.Add(newElement));

                // get an interface with a public IP address, then we will have a proper MAC address..
                Core::IPV4AddressIterator selectedNode(interfaces.Index());

                while (selectedNode.Next() == true) {
                    Core::JSON::String nodeName;
                    nodeName = selectedNode.Address().HostAddress();

                    element.IPAddress.Add(nodeName);
                }
            }
        }

        void DeviceInfo::SocketPortInfo(Data::SocketPortInfo& socketPortInfo)
        {
#ifdef __DEBUG__
            socketPortInfo.Total = Core::SocketPort::SocketsInState(static_cast<Core::SocketPort::enumState>(0));
            socketPortInfo.Open = Core::SocketPort::SocketsInState(Core::SocketPort::OPEN);
            socketPortInfo.Link = Core::SocketPort::SocketsInState(Core::SocketPort::LINK);
            socketPortInfo.Exception = Core::SocketPort::SocketsInState(Core::SocketPort::EXCEPTION);
            socketPortInfo.Shutdown = Core::SocketPort::SocketsInState(Core::SocketPort::SHUTDOWN);
#ifdef SOCKET_TEST_VECTORS
            socketPortInfo.Runs = Core::SocketPort::MonitorRuns();
#endif
#else
            socketPortInfo.Total = 0;
            socketPortInfo.Open = 0;
            socketPortInfo.Link = 0;
            socketPortInfo.Exception = 0;
            socketPortInfo.Shutdown = 0;
            socketPortInfo.Runs = 0;
#endif
        }

        string DeviceInfo::GetDeviceId() const
        {
            string result;

            const PluginHost::ISubSystem::IIdentifier* info (_subSystem->Get<PluginHost::ISubSystem::IIdentifier>());

            if (info != nullptr)
            {
                uint8_t myBuffer[64];

                myBuffer[0] = info->Identifier(sizeof(myBuffer)-1, &(myBuffer[1]));

                info->Release();

                if (myBuffer[0] != 0)
                {
                    result = Core::SystemInfo::Instance().Id(myBuffer, ~0);
                }
            }

            return (result);
        }

    } // namespace Plugin
} // namespace WPEFramework
