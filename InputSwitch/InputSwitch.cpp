#include "InputSwitch.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(InputSwitch, 1, 0);

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<InputSwitch::Data>> jsonResponseFactory(4);

    /* virtual */ const string InputSwitch::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);

        ASSERT(_subSystem == nullptr);

        Config config;
        config.FromString(service->ConfigLine());
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        _subSystem = service->SubSystems();
        _service = service;
        _systemId = Core::SystemInfo::Instance().Id(Core::SystemInfo::Instance().RawDeviceId(), ~0);

        ASSERT(_subSystem != nullptr);

        // On success return empty, to indicate there is no error text.

        return (_subSystem != nullptr) ? EMPTY_STRING : _T("Could not retrieve System Information.");
    }

    /* virtual */ void InputSwitch::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        if (_subSystem != nullptr) {
            _subSystem->Release();
            _subSystem = nullptr;
        }

        _service = nullptr;
    }

    /* virtual */ string InputSwitch::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void InputSwitch::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> InputSwitch::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        // <GET> - currently, only the GET command is supported, returning system info
        if (request.Verb == Web::Request::HTTP_GET) {

            Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());

            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

            // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
            index.Next();

            if (index.Next() == false) {
                AddressInfo(response->Addresses);
                SysInfo(response->SystemInfo);
                SocketPortInfo(response->Sockets);
            } else if (index.Current() == "Adresses") {
                AddressInfo(response->Addresses);
            } else if (index.Current() == "System") {
                SysInfo(response->SystemInfo);
            } else if (index.Current() == "Sockets") {
                SocketPortInfo(response->Sockets);
            }
            // TODO RB: I guess we should do something here to return other info (e.g. time) as well.

            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::proxy_cast<Web::IBody>(response));
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported request for the [InputSwitch] service.");
        }

        return result;
    }

    void InputSwitch::SysInfo(JsonData::InputSwitch::SysteminfoData& systemInfo) const
    {
        Core::SystemInfo& singleton(Core::SystemInfo::Instance());

        systemInfo.Time = Core::Time::Now().ToRFC1123(true);
        systemInfo.Version = _service->Version() + _T("#") + _subSystem->BuildTreeHash();
        systemInfo.Uptime = singleton.GetUpTime();
        systemInfo.Freeram = singleton.GetFreeRam();
        systemInfo.Totalram = singleton.GetTotalRam();
        systemInfo.Devicename = singleton.GetHostName();
        systemInfo.Cpuload = Core::NumberType<uint32_t>(static_cast<uint32_t>(singleton.GetCpuLoad())).Text();
        systemInfo.Serialnumber = _systemId;
    }

    void InputSwitch::AddressInfo(Core::JSON::ArrayType<JsonData::InputSwitch::AddressesData>& addressInfo) const
    {
        // Get the point of entry on WPEFramework..
        Core::AdapterIterator interfaces;

        while (interfaces.Next() == true) {

            JsonData::InputSwitch::AddressesData newElement;
            newElement.Name = interfaces.Name();
            newElement.Mac = interfaces.MACAddress(':');
            JsonData::InputSwitch::AddressesData& element(addressInfo.Add(newElement));

            // get an interface with a public IP address, then we will have a proper MAC address..
            Core::IPV4AddressIterator selectedNode(interfaces.Index());

            while (selectedNode.Next() == true) {
                Core::JSON::String nodeName;
                nodeName = selectedNode.Address().HostAddress();

                element.Ip.Add(nodeName);
            }
        }
    }

    void InputSwitch::SocketPortInfo(JsonData::InputSwitch::SocketinfoData& socketPortInfo) const
    {
        socketPortInfo.Runs = Core::ResourceMonitor::Instance().Runs();
    }

} // namespace Plugin
} // namespace WPEFramework
