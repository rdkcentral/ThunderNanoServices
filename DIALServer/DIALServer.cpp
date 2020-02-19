/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "DIALServer.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DIALServer, 1, 0);

    static const string _SearchTarget(_T("urn:dial-multiscreen-org:service:dial:1"));
    static const string _DefaultAppInfoPath(_T("Apps"));
    static const string _DefaultDataExtension(_T("Data"));
    static const string _DefaultDataUrlExtension(_T("DataUrl"));
    static const string _DefaultControlExtension(_T("Run"));
    static const string _DefaultAppInfoDevice(_T("DeviceInfo.xml"));
    static const string _DefaultRunningExtension(_T("Running"));
    static const string _SystemApp(_T("system"));

    constexpr char kHideCommand[] = "hide";
    static const string kVersionSupportedByClientQueryKey(_T("clientDialVer"));
    static const string kClientFriendyNameQueryKey = (_T("friendlyName"));
    constexpr int kMaxQueryValueLength = 256;

    static Core::ProxyPoolType<Web::TextBody> _textBodies(5);

    /* static */ const Core::NodeId DIALServer::DIALServerImpl::DialServerInterface(_T("239.255.255.250"), 1900);
    /* static */ std::map<string, DIALServer::IApplicationFactory*> DIALServer::AppInformation::_applicationFactory;

    class WebFlow {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        WebFlow(const WebFlow& a_Copy) = delete;
        WebFlow& operator=(const WebFlow& a_RHS) = delete;

    public:
        WebFlow(const Core::ProxyType<Web::Request>& request, const Core::NodeId& nodeId)
        {
            if (request.IsValid() == true) {
                string text;
                request->ToString(text);
                _text = Core::ToString(string("\n[" + nodeId.HostAddress() + ']' + text + '\n'));
            }
        }
        WebFlow(const Core::ProxyType<Web::Response>& response, const Core::NodeId& nodeId)
        {
            if (response.IsValid() == true) {
                string text;
                response->ToString(text);
                _text = Core::ToString(string("\n[" + nodeId.HostAddress() + ']' + text + '\n'));
            }
        }
        ~WebFlow()
        {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

    uint16_t DIALServer::WebTransform::Transform(Web::Request::Deserializer& deserializer, uint8_t* dataFrame, const uint16_t maxSendSize)
    {
        uint16_t index = 0;
        const uint8_t* checker = dataFrame;

        // This is a UDP service, so a message should be complete. If the first keyword is not a keyword we
        // expect, ignore the full message, it is not a DIAL server package and does not require any further
        // processing.
        // First skip the white sace, if applicable...
        while (isspace(*checker)) {
            checker++;
        }

        // Now see if the first keyword is "M-SEARCH"
        while ((index < _keywordLength) && (toupper(*checker) == Web::Request::MSEARCH[index])) {
            checker++;
            index++;
        }

        // Now we need to find the keyword "M-SEARCH" see if we have it..
        if (index == _keywordLength) {
            deserializer.Flush();
            index = deserializer.Deserialize(dataFrame, maxSendSize);
        } else {
            index = maxSendSize;
        }

        return (index);
    }

    DIALServer::DIALServerImpl::DIALServerImpl(const string& MACAddress, const string& baseURL, const string& appPath)
        : BaseClass(5, false, Core::NodeId(DialServerInterface.AnyInterface(), DialServerInterface.PortNumber()), DialServerInterface.AnyInterface(), 1024, 1024)
        , _response(Core::ProxyType<Web::Response>::Create())
        , _destinations()
        , _baseURL(baseURL)
        , _appPath(appPath)
    {
        _response->ErrorCode = Web::STATUS_OK;
        _response->Message = _T("OK");
        _response->CacheControl = _T("max-age=1800");
        _response->Server = _T("Linux/2.6 UPnP/1.0 quick_ssdp/1.0");
        _response->ST = _SearchTarget;
        _response->USN = _T("uuid:UniqueIdentifier::") + _SearchTarget;
        // FIXME: Uncomment when adding WoL/WoWLAN support.
        // This SHALL NOT be present if neither WoL nor WoWLAN is supported.
        // Moreover real MAC address of the network iface (either wired or wireless one) should be passed
        // where currently Device identifier is passed in MACAddress.
        // _response->WakeUp = _T("MAC=") + MACAddress + _T(";Timeout=10");
        _response->Mode(Web::MARSHAL_UPPERCASE);

        if (Link().Open(1000) != Core::ERROR_NONE) {
            ASSERT(false && "Seems we can not open the DIAL discovery port");
        }

        Link().Join(DialServerInterface);
    }

    /* virtual */ DIALServer::DIALServerImpl::~DIALServerImpl()
    {
        Link().Leave(DialServerInterface);
        Link().Close(Core::infinite);
    }

    // Notification of a Partial Request received, time to attach a body..
    /* virtual */ void DIALServer::DIALServerImpl::LinkBody(Core::ProxyType<Web::Request>& element)
    {
        // upnp requests are empty so no body needed...
    }
    // Notification of a Partial Request received, time to attach a body..
    // upnp requests are empty so no body needed...
    /* virtual */ void DIALServer::DIALServerImpl::Received(Core::ProxyType<Web::Request>& request)
    {
        Core::NodeId sourceNode(Link().ReceivedNode());
        TRACE(WebFlow, (request, sourceNode));

        if ((request->Verb == Web::Request::HTTP_MSEARCH) && (request->ST.IsSet() == true)) {
            if (request->ST.Value() == _SearchTarget) {

                TRACE(Protocol, (&(*request)));
                _destinations.push_back(sourceNode);
                // remember the NodeId where this comes from.
                if (_destinations.size() == 1) {

                    Link().RemoteNode(_destinations.front());

                    _response->Location = URL() + '/' + _DefaultAppInfoDevice;

                    Submit(_response);
                }
            }
        }
    }

    // Notification of a Response send.
    /* virtual */ void DIALServer::DIALServerImpl::Send(const Core::ProxyType<Web::Response>& response)
    {
        TRACE(WebFlow, (response, _destinations.front()));

        TRACE(Protocol, (&(*response)));

        // Drop the current destination.
        _destinations.pop_front();

        if (_destinations.size() > 0) {
            _response->Location = URL() + '/' + _DefaultAppInfoDevice;

            // Move on to notifying the next one.
            Link().RemoteNode(_destinations.front());

            Submit(response);
        }
    }

    // Notification of a channel state change..
    /* virtual */ void DIALServer::DIALServerImpl::StateChange()
    {
    }

    void DIALServer::AppInformation::GetData(string& data, const Version& version) const
    {
        bool running = IsRunning();
        bool hidden = HasHideAndShow() == true && IsHidden() == true;
        bool isAtLeast2_1 = Version{2, 1, 0} <= version;
        // allowSop is mandatory to be true starting from 2.1
        string allowStop = isAtLeast2_1 == true || HasStartAndStop() == true ? "true" : "false";
        string dialVersion;
        if (isAtLeast2_1 == true) {
          dialVersion = " dialVer=\"2.1\" ";
        }

        data = _T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")
               _T("<service xmlns=\"urn:dial-multiscreen-org:schemas:dial\"") + dialVersion + _T(">")
               _T("<name>")
            + Name() + _T("</name>")
                       _T("<options allowStop=\"")
            + allowStop + _T("\"/><state>")
            // 2.2 spec adds "hidden" state. It also introduces "installable" state.
            // We may consider adding support for it at some point - thanks to the Packager.
            + (running == true ? (hidden == true && isAtLeast2_1 == true ? _T("hidden") : _T("running")) : _T("stopped")) + _T("</state>")
            // <link> element is DEPRECATED starting from 2.1
            + (running == true && isAtLeast2_1 == false ? _T("<link rel=\"run\" href=\"" + _DefaultControlExtension + "\"/>") : _T(""))
            + _T("<additionalData>");
        auto additionalData = AdditionalData();
        if (additionalData.empty() == false) {
            for (const auto& adata : additionalData) {
                // FIXME: escape/validate keys and values
                data += "<" + adata.first + ">"+ adata.second + "</" + adata.first + ">";
            }
        }
        data += _T("</additionalData></service>");
    }

    void DIALServer::AppInformation::SetData(const string& data)
    {
        TRACE(Trace::Information, (_T("SetData: %s"), data.c_str()));
        TCHAR* decoded = static_cast<TCHAR*>(ALLOCA(MaxDialQuerySize * sizeof(TCHAR)));

        IApplication::AdditionalDataType additionalData;
        _lock.Lock();

        Core::TextSegmentIterator external(Core::TextFragment(data), true, '&');

        while (external.Next() == true) {
            Core::TextSegmentIterator internal(external.Current(), true, '=');

            if (internal.Next() == true) {
                uint16_t length = Core::URL::Decode(internal.Current().Data(), internal.Current().Length(), decoded, MaxDialQuerySize);
                string key(decoded, length);

                if (internal.Next() == true) {
                    length = Core::URL::Decode(internal.Current().Data(), internal.Current().Length(), decoded, MaxDialQuerySize);
                    string value(decoded, length);
                    additionalData.emplace(key, value);
                }
            }
        }

        _application->AdditionalData(std::move(additionalData));
        _lock.Unlock();
    }

    /* virtual */ const string DIALServer::Initialize(PluginHost::IShell* service)
    {
        string result;

        ASSERT(_service == NULL);

        _config.FromString(service->ConfigLine());

        // get an interface with a public IP address, then we will have a proper MAC address..
        Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(_config.Interface.Value());

        if (selectedNode.IsValid() == false) {
            // Oops no way we can operate...
            result = _T("No DIALInterface available.");
        } else {
            const uint8_t* rawId(Core::SystemInfo::Instance().RawDeviceId());
            const string deviceId(Core::SystemInfo::Instance().Id(rawId, ~0));

            Core::NodeId source((_config.Interface.Value().empty() == true) || (selectedNode.IsValid() == false) ? selectedNode.AnyInterface() : selectedNode);

            _dialURL = Core::URL(service->Accessor());
            _dialPath = '/' + _dialURL.Path().Value().Text();

            // TODO: THis used to be the MAC, but I think  it is just a unique number, otherwise, we need the MAC
            //       that goes with the selectedNode !!!!
            _dialServiceImpl = new DIALServerImpl(deviceId, service->Accessor(), _DefaultAppInfoPath);

            ASSERT(_dialServiceImpl != nullptr);

            _skipURL = static_cast<uint16_t>(service->WebPrefix().length());

            (*_deviceInfo) = _T("<?xml version=\"1.0\"?>")
                             _T("<root xmlns=\"urn:schemas-upnp-org:device-1-0\">")
                             _T("<specVersion>")
                             _T("<major>1</major>")
                             _T("<minor>0</minor>")
                             _T("</specVersion>")
                             _T("<device>")
                             _T("<deviceType>urn:schemas-upnp-org:device:tvdevice:1</deviceType>")
                             _T("<friendlyName>")
                + _config.Name.Value() + _T("</friendlyName>")
                                         _T("<manufacturer>")
                + _config.Manufacturer.Value() + _T("</manufacturer>")
                + ( _config.ManufacturerURL.IsSet() == true ? _T("<manufacturerURL>") + _config.ManufacturerURL.Value() + _T("</manufacturerURL>") : _T("") )
                +                                 _T("<modelDescription>")
                + _config.Description.Value() + _T("</modelDescription>")
                                                _T("<modelName>")
                + _config.Model.Value() + _T("</modelName>")
                + ( _config.ModelNumber.IsSet() == true ? _T("<modelNumber>") + _config.ModelNumber.Value() + _T("</modelNumber>") : _T("") )
                + ( _config.ModelURL.IsSet() == true ? _T("<modelURL>") + _config.ModelURL.Value() + _T("</modelURL>") : _T("") )
                + ( _config.SerialNumber.IsSet() == true ? _T("<serialNumber>") + _config.SerialNumber.Value() + _T("</serialNumber>") : _T("") )
                +                          _T("<UDN>uuid:")
                + deviceId + _T("</UDN>")
                + ( _config.UPC.IsSet() == true ? _T("<UPC>") + _config.UPC.Value() + _T("</UPC>") : _T("") )
                +            _T("</device>")
                             _T("</root>");

            // Create a list of all servicable Apps:
            auto index(_config.Apps.Elements());

            while (index.Next() == true) {
                if (index.Current().Name.IsSet() == true) {
                    _appInfo.emplace(std::piecewise_construct, std::make_tuple(index.Current().Name.Value()), std::make_tuple(service, index.Current(), this));
                }
            }

            AppInformation::Announce(_SystemApp, new SystemApplicationFactory);
            Config::App systemCfg;
            systemCfg.Name = _SystemApp;
            _appInfo.emplace(std::piecewise_construct, std::make_tuple(_SystemApp), std::make_tuple(service, systemCfg, this));

            _service = service;

            _adminLock.Lock();

            // Register the sink *AFTER* the apps have been created so the apps will receive the
            // switchboard, if it is already active and configured !!!
            _sink.Register(service, _config.WebServer.Value(), _config.SwitchBoard.Value());

            _adminLock.Unlock();
        }

        // On succes return NULL, to indicate there is no error text.
        return (result);
    }

    /* virtual */ void DIALServer::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service != NULL);
        ASSERT(_dialServiceImpl != NULL);

        _adminLock.Lock();

        AppInformation::Revoke(_SystemApp);

        _sink.Unregister(service);

        _adminLock.Unlock();

        delete _dialServiceImpl;
        _dialServiceImpl = nullptr;

        _appInfo.clear();

        _service = nullptr;
    }

    /* virtual */ string DIALServer::Information() const
    {
        // No additional info to report.
        return (string());
    }
    /* virtual */ void DIALServer::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_POST) {
            // This might be a "launch" application thingy, make sure we reive the proper info.
            request.Body(_textBodies.Element());
        }
    }

    void DIALServer::StartApplication(const Web::Request& request, Core::ProxyType<Web::Response>& response, AppInformation& app)
    {
        string parameters;
        string additioanlDataUrl =  "http://localhost/" + app.Name() + "/" + _DefaultDataExtension;
        TCHAR* encodedDataUrl = static_cast<TCHAR*>(ALLOCA(additioanlDataUrl.length() * 3 * sizeof(TCHAR)));
        Core::URL::Encode(additioanlDataUrl.c_str(), static_cast<uint16_t>(additioanlDataUrl.length()), encodedDataUrl, static_cast<uint16_t>(additioanlDataUrl.length() * 3 * sizeof(TCHAR)));
        if (request.HasBody() == true) {
            parameters = *request.Body<const Web::TextBody>();
            parameters = app.AppURL() + "?" + parameters;
            parameters += "&additionalDataUrl=";
        } else {
          parameters = app.AppURL() + "?additionalDataUrl=";
        }
        parameters += encodedDataUrl;

        if (parameters.length() > MaxDialQuerySize) {
            response->ErrorCode = Web::STATUS_REQUEST_ENTITY_TOO_LARGE;
            response->Message = _T("Request entity is too big to fit the URL");
        } else {
            TRACE(Trace::Information, (_T("Launch Application [%s] with params: %s"), app.Name().c_str(), parameters.c_str()));

            // See if we can find the plugin..
            ASSERT(_service != NULL);

            if (app.IsRunning() == false) {
                if (app.HasStartAndStop() == false) {
                    response->ErrorCode = Web::STATUS_FORBIDDEN;
                    response->Message = _T("Forbidden");
                } else {
                    uint32_t result = app.Start(parameters);
                    if (result != Core::ERROR_NONE && result != Core::ERROR_INPROGRESS) {
                        response->ErrorCode = Web::STATUS_SERVICE_UNAVAILABLE;
                        response->Message = _T("Service Unavailable");
                    } else {
                        response->Location = _dialServiceImpl->URL() + '/' + app.Name() + '/' + _DefaultControlExtension;
                        response->ErrorCode = Web::STATUS_CREATED;
                        response->Message = _T("Created");
                    }
                }
            } else {
                if (app.HasHideAndShow() == true && app.IsHidden() == true) {
                    uint32_t result = app.Show(parameters);
                    // system app has special error codes. Handle them here.
                    if (app.Name() == _SystemApp) {
                        if (result == Core::ERROR_NONE) {
                            response->ErrorCode = Web::STATUS_OK;
                            response->Message = _T("OK");
                        } else if (result == Core::ERROR_UNKNOWN_KEY) {
                            response->ErrorCode = Web::STATUS_FORBIDDEN;
                            response->Message = _T("Forbidden");
                        } else {
                            response->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                            response->Message = _T("Internal error");
                        }
                    } else {
                      if (result == Core::ERROR_NONE) {
                          response->Location = _dialServiceImpl->URL() + '/' + app.Name() + '/' + _DefaultControlExtension;
                          response->ErrorCode = Web::STATUS_CREATED;
                          response->Message = _T("Created");
                      } else {
                          response->ErrorCode = Web::STATUS_SERVICE_UNAVAILABLE;
                          response->Message = _T("Service Unavailable");
                      }
                    }
                } else {
                    if (request.HasBody() == true) {
                        // Providing new arguments to the running app is not implemented.
                        response->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                        response->Message = _T("Not Implemented");
                    } else {
                        response->ErrorCode = Web::STATUS_OK;
                        response->Message = _T("OK");
                    }
                }
            }
        }
    }

    void DIALServer::StopApplication(const Web::Request& request, Core::ProxyType<Web::Response>& response, AppInformation& app)
    {
        if (app.IsRunning() == false) {
            response->ErrorCode = Web::STATUS_NOT_FOUND;
            response->Message = _T("Requested app is not running.");
        } else {
            if (app.HasStartAndStop() == false) {
                response->ErrorCode = Web::STATUS_FORBIDDEN;
                response->Message = _T("Forbidden");
            } else {
                response->ErrorCode = Web::STATUS_OK;
                response->Message = _T("OK");
                app.Stop(_T(""));
            }
        }
    }

    // <GET> ../
    /* virtual */ Core::ProxyType<Web::Response> DIALServer::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());
        // <GET> ../
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint16_t>(request.Path.length()) - _skipURL), false, '/');

        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
        result->Message = string(_T("Unknown request path specified."));

        TRACE(Protocol, (&request));

        // If there is nothing or only a slashe, we will now jump over it, and otherwise, we have data.
        if ((index.Next() == true) && (index.Current() == _DefaultAppInfoPath) && (index.Next() == true)) {
            string keyword(index.Current().Text());

            if (keyword == _DefaultAppInfoDevice) {
                if (request.Verb != Web::Request::HTTP_GET) {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = string(_T("The Device Description file can only be retrieved (GET)."));
                } else {
                    TRACE(Trace::Information, (string(_T("Serving the Device Description File"))));

                    // Serve the Device file
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("OK");
                    result->Body(_deviceInfo);
                    result->ContentType = Web::MIME_XML;

                    Core::URL newURL;
                    _dialServiceImpl->URL(newURL);

                    result->ApplicationURL = newURL;
                    TRACE(Protocol, (static_cast<const string&>(*_deviceInfo), &newURL));
                }
            } else {
                auto selectedApp(_appInfo.find(keyword));

                if (selectedApp == _appInfo.end()) {
                    result->ErrorCode = Web::STATUS_NOT_FOUND;
                    result->Message = _T("Requested App Description file [") + keyword + _T("] not found.");
                } else if (index.Next() == false) {

                    Version version;
                    std::array<char, kMaxQueryValueLength> clientVersion {0};
                    Core::URL::KeyValue options(request.Query.Value());
                    if (options.Exists(kVersionSupportedByClientQueryKey.c_str(), true) == true) {
                        const string versionStr (options[kVersionSupportedByClientQueryKey.c_str()].Text());
                        Core::URL::Decode(versionStr.c_str(), static_cast<uint16_t>(versionStr.length()), clientVersion.data(), static_cast<uint16_t>(clientVersion.size()));

                        DIALServer::ParseVersion(versionStr, &version);
                    }

                    if (options.Exists(kClientFriendyNameQueryKey.c_str(), true) == true) {
                        if (version.IsValid() == false || (version.IsDefault() == true &&
                                                           version < Version{2, 1, 0})) {
                            // No version was specified but firendlyName was and this
                            // may only be sent to Server 2.1+
                            version.Major = 2;
                            version.Minor = 1;
                            version.Patch = 0;
                        }
                    }

                    if (version.IsValid() == false) {
                        version.SetDefault();
                    }

                    // Get / Set AppInformation
                    if (request.Verb == Web::Request::HTTP_GET) {
                        // We are at the end.. this is getting App info
                        TRACE(Trace::Information, (_T("Serving the Application [%s] Description File"), selectedApp->second.Name().c_str()));

                        Core::ProxyType<Web::TextBody> info(_textBodies.Element());
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK");
                        result->ContentType = Web::MIME_XML;
                        Core::ProxyType<Web::TextBody> textBody(_textBodies.Element());
                        selectedApp->second.GetData(*textBody, version);
                        result->Body(textBody);
                        TRACE(Protocol, (static_cast<const string&>(*textBody)));
                    } else if (request.Verb == Web::Request::HTTP_POST) {
                        StartApplication(request, result, selectedApp->second);
                    }
                } else if (index.Current() == _DefaultDataUrlExtension) {
                    if (request.Verb == Web::Request::HTTP_GET) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK");

                        Core::ProxyType<Web::TextBody> message(_textBodies.Element());
                        *message = string("{") + "\"dataUrl\":\"http://localhost/" + selectedApp->second.Name() + "/" + _DefaultDataExtension + "\"" + "}";
                        result->Body(message);
                    }
                } else if (index.Current() == _DefaultControlExtension) {
                    if (request.Verb == Web::Request::HTTP_DELETE) {
                        StopApplication(request, result, selectedApp->second);
                    } else if (request.Verb == Web::Request::HTTP_POST) {
                      if (index.Next() == true && index.Current() == kHideCommand) {
                          if (selectedApp->second.HasHideAndShow() == true) {
                              result->ErrorCode = Web::STATUS_OK;
                              result->Message = _T("OK");
                              selectedApp->second.Hide();
                          } else {
                              result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                              result->Message = _T("Not Implemented");
                          }
                      } else {
                          StartApplication(request, result, selectedApp->second);
                      }
                    }
                } else if (index.Current() == _DefaultRunningExtension) {
                    if ((request.Verb == Web::Request::HTTP_POST) || (request.Verb != Web::Request::HTTP_DELETE)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK");
                        selectedApp->second.Running(request.Verb == Web::Request::HTTP_POST);
                    }
                } else if (index.Current() == _DefaultDataExtension) {
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("OK");

                    // Data time!!
                    if (request.Verb == Web::Request::HTTP_GET) {
                        selectedApp->second.SetData(request.Query.Value());
                    } else if (request.Verb == Web::Request::HTTP_POST) {
                        selectedApp->second.SetData(static_cast<const string&>(*(request.Body<const Web::TextBody>())));
                    } else if (request.Verb == Web::Request::HTTP_DELETE) {
                        selectedApp->second.SetData(EMPTY_STRING);
                    }
                }
            }
        }

        if (request.Origin.IsSet() == true)
            result->AccessControlOrigin = request.Origin.Value();

        TRACE(Protocol, (&(*result)));
        return (result);
    }

    void DIALServer::Activated(Exchange::IWebServer* pluginInterface)
    {
        string remote(_dialURL.Host().Value().Text() + ':' + (_dialURL.Port().IsSet() ? Core::NumberType<uint16_t>(_dialURL.Port().Value()).Text() : _T("80")));

        _adminLock.Lock();

        // Let's set the URL of the WebServer, as it is active :-)
        _dialServiceImpl->Locator(pluginInterface->Accessor() + _dialPath);

        // Redirect all calls to the DIALServer, via a proxy.
        pluginInterface->AddProxy(_dialPath, _dialPath, remote);

        _adminLock.Unlock();
    }

    void DIALServer::Deactivated(Exchange::IWebServer* pluginInterface)
    {
        ASSERT(_dialServiceImpl != nullptr);

        _adminLock.Lock();

        _dialServiceImpl->Locator(_dialURL.Text().Text());
        pluginInterface->RemoveProxy(_dialPath);

        _adminLock.Unlock();
    }

    void DIALServer::Activated(Exchange::ISwitchBoard* switchBoard)
    {

        _adminLock.Lock();

        std::map<const string, AppInformation>::iterator index(_appInfo.begin());

        while (index != _appInfo.end()) {
            index->second.SwitchBoard(switchBoard);
            index++;
        }

        _adminLock.Unlock();
    }

    void DIALServer::Deactivated(Exchange::ISwitchBoard* /* switchBoard */)
    {

        _adminLock.Lock();

        std::map<const string, AppInformation>::iterator index(_appInfo.begin());

        while (index != _appInfo.end()) {
            index->second.SwitchBoard(nullptr);
            index++;
        }

        _adminLock.Unlock();
    }
}
}
