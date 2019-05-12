#include "DeviceInfo.h"

#ifdef PROVIDE_NEXUS_OTP_ID
#include <nexus_config.h>
#include <nxclient.h>
#if NEXUS_SECURITY_API_VERSION == 2
#include "nexus_otp_key.h"
#else
#include "nexus_otpmsp.h"
#include "nexus_read_otp_id.h"
#endif
#endif

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceInfo, 1, 0);

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<DeviceInfo::Data>> jsonResponseFactory(4);

    /* virtual */ const string DeviceInfo::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);

        ASSERT(_subSystem == nullptr);

        Config config;
        config.FromString(service->ConfigLine());
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        _subSystem = service->SubSystems();
        _service = service;
#ifdef PROVIDE_NEXUS_OTP_ID
        ASSERT(_idProvider == nullptr);
        _idProvider = Core::Service<IdentityProvider>::Create<IdentityProvider>();
        _subSystem->Set(PluginHost::ISubSystem::IDENTIFIER, _idProvider);
#endif
        _deviceId = GetDeviceId();
        _systemId = Core::SystemInfo::Instance().Id(Core::SystemInfo::Instance().RawDeviceId(), ~0);

        ASSERT(_subSystem != nullptr);

        // On success return empty, to indicate there is no error text.

        return (_subSystem != nullptr) ? EMPTY_STRING : _T("Could not retrieve System Information.");
    }

    /* virtual */ void DeviceInfo::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        if (_idProvider != nullptr) {
            delete _idProvider;
            _idProvider = nullptr;
        }

        if (_subSystem != nullptr) {
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
            result->Message = _T("Unsupported request for the [DeviceInfo] service.");
        }

        return result;
    }

    void DeviceInfo::SysInfo(Data::SysInfo& systemInfo)
    {
        Core::SystemInfo& singleton(Core::SystemInfo::Instance());

        if (_deviceId.empty() == true) {
            _deviceId = GetDeviceId();
        }
        if (_deviceId.empty() == false) {
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
        socketPortInfo.Runs = Core::ResourceMonitor::Instance().Runs();
    }

    string DeviceInfo::GetDeviceId() const
    {
        string result;

        const PluginHost::ISubSystem::IIdentifier* info(_subSystem->Get<PluginHost::ISubSystem::IIdentifier>());

        if (info != nullptr) {
            uint8_t myBuffer[64];

            myBuffer[0] = info->Identifier(sizeof(myBuffer) - 1, &(myBuffer[1]));

            info->Release();

            if (myBuffer[0] != 0) {
                result = Core::SystemInfo::Instance().Id(myBuffer, ~0);
            }
        }

        return (result);
    }

#ifdef PROVIDE_NEXUS_OTP_ID
    DeviceInfo::IdentityProvider::IdentityProvider()
        : _identifier(nullptr)
    {
        ASSERT(_identifier == nullptr);
        if (_identifier == nullptr) {
            if (NEXUS_SUCCESS == NxClient_Join(NULL))
            {
#if NEXUS_SECURITY_API_VERSION == 2
                NEXUS_OtpKeyInfo keyInfo;
                if (NEXUS_SUCCESS == NEXUS_OtpKey_GetInfo(0 /*key A*/, &keyInfo)){
                    _identifier = new uint8_t[NEXUS_OTP_KEY_ID_LENGTH + 2];

                    ::memcpy(&(_identifier[1]), keyInfo.id, NEXUS_OTP_KEY_ID_LENGTH);

                    _identifier[0] = NEXUS_OTP_KEY_ID_LENGTH;
                    _identifier[NEXUS_OTP_KEY_ID_LENGTH + 1] = '\0';
                }
#else
                NEXUS_OtpIdOutput id;
                if (NEXUS_SUCCESS == NEXUS_Security_ReadOtpId(NEXUS_OtpIdType_eA, &id) ) {
                    _identifier = new uint8_t[id.size + 2];

                    ::memcpy(&(_identifier[1]), id.otpId, id.size);

                    _identifier[0] = id.size;
                    _identifier[id.size + 1] = '\0';
                }
#endif
                NxClient_Uninit();
            }
        }
    }
#endif
} // namespace Plugin
} // namespace WPEFramework
