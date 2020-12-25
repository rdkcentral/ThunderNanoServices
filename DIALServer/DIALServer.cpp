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
    static const string _DefaultHiddenExtension(_T("Hidden"));
    static const string _ClientFriendlyName = (_T("friendlyName"));

    constexpr TCHAR _VersionSupportedKey[] = _T("clientDialVer");
    constexpr TCHAR _HideCommand[] = _T("hide");

    static Core::ProxyPoolType<Web::TextBody> _textBodies(5);

    /* static */ const Core::NodeId DIALServer::DIALServerImpl::DialServerInterface(_T("239.255.255.250"), 1900);
    /* static */ std::map<string, DIALServer::IApplication::IFactory*> DIALServer::AppInformation::_applicationFactory;

    class WebFlow {
    public:
        WebFlow(const WebFlow& a_Copy) = delete;
        WebFlow& operator=(const WebFlow& a_RHS) = delete;

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
        ~WebFlow() = default;

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
        static string dialVersion(_T(" dialVer=\"") + Core::NumberType<uint8_t>(DIALServer::DialServerMajor).Text() + _T(".") + Core::NumberType<uint8_t>(DIALServer::DialServerMinor).Text() + _T("\" "));

        bool running = IsRunning();
        bool isAtLeast2_1 = version >= Version(2, 1, 0);

        // 2.1 spec adds "hidden" state. It also introduces "installable" state.
        // We may consider adding support for it at some point - thanks to the Packager.
        string state(((isAtLeast2_1 == true) && (HasHide() == true) && (IsHidden() == true) ? _T("hidden") : ((running == true) ? _T("running") : _T("stopped"))));

        // allowStop is mandatory to be true starting from 2.1
        string allowStop((isAtLeast2_1 == true) || (HasStartAndStop() == true) ? "true" : "false");

        // <link> element is DEPRECATED starting from 2.1!!!!
        // Although it is deperecated some Cobalt tests are still checking for the presence of this element. Keep on adding it. It does not hurt.... 

        data = _T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")
            _T("<service xmlns=\"urn:dial-multiscreen-org:schemas:dial\"") + dialVersion + _T(">")
            _T("<name>") + Name() + _T("</name>")
            _T("<options allowStop=\"") + allowStop + _T("\"/>")
            _T("<state>") + state + _T("</state>")
            _T("<link rel=\"run\" href=\"" + _DefaultControlExtension + "\"/>")
            _T("<additionalData>");

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
        TCHAR decoded[MaxDialQuerySize * sizeof(TCHAR)];

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
        _deprecatedAPI = _config.DeprecatedAPI.Value();

        // get an interface with a public IP address, then we will have a proper MAC address..
        // Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(_config.Interface.Value());
        Core::NodeId selectedNode = Core::NodeId(_config.Interface.Value().c_str());

        if (selectedNode.IsValid() == false) {
            // Oops no way we can operate...
            result = _T("No DIALInterface available.");
        } else {
            const uint8_t* rawId(Core::SystemInfo::Instance().RawDeviceId());
            const string deviceId(Core::SystemInfo::Instance().Id(rawId, ~0));

            _service = service;
            _dialURL = Core::URL(service->Accessor());
            _dialURL.Host(selectedNode.HostAddress());
            _dialPath = '/' + _dialURL.Path().Value();
            _webServerPort = _dialURL.Port();

            // TODO: THis used to be the MAC, but I think  it is just a unique number, otherwise, we need the MAC
            //       that goes with the selectedNode !!!!
            _dialServiceImpl = new DIALServerImpl(deviceId, _dialURL.Text(), _DefaultAppInfoPath);

            ASSERT(_dialServiceImpl != nullptr);

            _skipURL = static_cast<uint16_t>(service->WebPrefix().length());

            (*_deviceInfo) = _T("<?xml version=\"1.0\"?>")
                             _T("<root xmlns=\"urn:schemas-upnp-org:device-1-0\">")
                                 _T("<specVersion>")
                                     _T("<major>") + Core::NumberType<uint8_t>(DialServerMajor).Text() + _T("</major>")
                                     _T("<minor>") + Core::NumberType<uint8_t>(DialServerMinor).Text() + _T("</minor>")
                                     _T("<patch>") + Core::NumberType<uint8_t>(DialServerPatch).Text() + _T("</patch>")
                                 _T("</specVersion>")
                                 _T("<device>")
                                     _T("<deviceType>urn:schemas-upnp-org:device:tvdevice:1</deviceType>")
                                     _T("<friendlyName>") + _config.Name.Value() + _T("</friendlyName>")
                                     _T("<manufacturer>") + _config.Manufacturer.Value() + _T("</manufacturer>") +
                                       ( _config.ManufacturerURL.IsSet() == true ? _T("<manufacturerURL>") + _config.ManufacturerURL.Value() + _T("</manufacturerURL>") : _T("") ) +
                                     _T("<modelDescription>") + _config.Description.Value() + _T("</modelDescription>") 
                                     _T("<modelName>") + _config.Model.Value() + _T("</modelName>") +
                                       ( _config.ModelNumber.IsSet() == true ? _T("<modelNumber>") + _config.ModelNumber.Value() + _T("</modelNumber>") : _T("") ) +
                                       ( _config.ModelURL.IsSet() == true ? _T("<modelURL>") + _config.ModelURL.Value() + _T("</modelURL>") : _T("") ) +
                                       ( _config.SerialNumber.IsSet() == true ? _T("<serialNumber>") + _config.SerialNumber.Value() + _T("</serialNumber>") : _T("") ) +
                                     _T("<UDN>uuid:") + deviceId + _T("</UDN>") +
                                       ( _config.UPC.IsSet() == true ? _T("<UPC>") + _config.UPC.Value() + _T("</UPC>") : _T("") ) +
                                 _T("</device>")
                             _T("</root>");

            // Create a list of all servicable Apps:
            auto index(_config.Apps.Elements());

            while (index.Next() == true) {
                if (index.Current().Name.IsSet() == true) {
                    _appInfo.emplace(std::piecewise_construct, std::make_tuple(index.Current().Name.Value()), std::make_tuple(service, index.Current(), this));
                }
            }

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
        // This might be a "launch" application thingy, make sure we reive the proper info.
        request.Body(_textBodies.Element());
    }

    void DIALServer::StartApplication(const Web::Request& request, Core::ProxyType<Web::Response>& response, AppInformation& app)
    {
        const string payload = ((request.HasBody() == true)? string(*request.Body<const Web::TextBody>()) : "");

        if (payload.length() > MaxDialQuerySize) {
            response->ErrorCode = Web::STATUS_REQUEST_ENTITY_TOO_LARGE;
            response->Message = _T("Payload too long");
        } else {
            // FIXME: At the moment part of additionalDataUrl parameter is hardcoded, localhost is obligatory by Netflix 
            // but rest of the path can be created dynamically or should be retrived from configuration    
            const string additionalDataUrl = (_T("http://localhost") + ((_webServerPort != 80)? _T("") : _T(":") + Core::NumberType<uint16_t>(_webServerPort).Text()) + _T("/Service/DIALServer/Apps/") + app.Name() + _T("/") + _DefaultDataExtension);
            const uint16_t maxEncodedSize = static_cast<uint16_t>(additionalDataUrl.length() * 3 * sizeof(TCHAR));
            TCHAR* encodedDataUrl = reinterpret_cast<TCHAR*>(ALLOCA(maxEncodedSize)); 
            uint16_t dialpayload = Core::URL::Encode(additionalDataUrl.c_str(), static_cast<uint16_t>(additionalDataUrl.length()), encodedDataUrl, maxEncodedSize);
            const string parameters = (app.AppURL() + (app.HasQueryParameter() ? _T("&") : _T("?")) + ((DeprecatedAPI() == true) ? (_T("dialpayload=") + std::to_string(dialpayload) + _T("&")) : _T("")) + _T("additionalDataUrl=") + encodedDataUrl);

            TRACE(Trace::Information, (_T("Launch Application [%s] with params: %s, payload: %s"), app.Name().c_str(), parameters.c_str(), payload.c_str()));

            // See if we can find the plugin..
            ASSERT(_service != NULL);

            if (app.IsRunning() == false) {
                if (app.HasStartAndStop() == false) {
                    response->ErrorCode = Web::STATUS_FORBIDDEN;
                    response->Message = _T("Forbidden");
                } else {
                    uint32_t result = app.Start(parameters, payload);
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
                // Make sure that there is connection between DIAL handler and application
                if (app.IsConnected() == false && app.Connect() == false) {
                    
                    TRACE(Trace::Information, (_T("Cannot connect DIAL handler to application %s"), app.Name().c_str()));
                } else if (app.HasHide() == true && app.IsHidden() == true) {
                    uint32_t result = (DeprecatedAPI() == true) ? app.Show() : app.Start(parameters, payload);

                    if (result == Core::ERROR_NONE) {
                        response->Location = _dialServiceImpl->URL() + '/' + app.Name() + '/' + _DefaultControlExtension;
                        response->ErrorCode = Web::STATUS_OK;
                        response->Message = _T("Created");
                    }
                    else {
                        response->ErrorCode = Web::STATUS_SERVICE_UNAVAILABLE;
                        response->Message = _T("Service Unavailable");
                    }
                } else {
                    if (request.HasBody() == true) {
                        if (app.URL(parameters, payload) == true) {
                            response->Location = _dialServiceImpl->URL() + '/' + app.Name() + '/' + _DefaultControlExtension;
                            response->ErrorCode = Web::STATUS_OK;
                            response->Message = _T("Created");
                        }
                        else {
                            response->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                            response->Message = _T("Can not change the URL runtime");
                        }
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
                app.Stop(_T(""), _T(""));
            }
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> DIALServer::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        // <GET> ../
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint16_t>(request.Path.length()) - _skipURL), false, '/');

        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        result->ErrorCode = Web::STATUS_NOT_FOUND;
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
                    result->ContentType = Web::MIME_TEXT_XML;

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
                }
                else if (SafeOrigin(request, selectedApp->second) == false) {
                    result->ErrorCode = Web::STATUS_FORBIDDEN;
                    result->Message = string(_T("Origin of the request is incorrect."));
                }
                else if (index.Next() == false) {

                    string versionText;
                    Core::URL::KeyValue options(request.Query.Value());
                    if (options.Exists(_VersionSupportedKey, true) == true) {
                        TCHAR destination[256];
                        versionText = options[_VersionSupportedKey].Text();
                        uint16_t length = Core::URL::Decode(versionText.c_str(), static_cast<uint16_t>(versionText.length()), destination, sizeof(destination));

                        versionText = string(destination, length);
                    }

                    Version version(versionText);

                    if (options.Exists(_ClientFriendlyName.c_str(), true) == true) {
                        if ((version.IsValid() == false) || (version < Version(2, 1, 0))) {
                            // No version was specified but firendlyName was and this
                            // may only be sent to Server 2.1+
                            version = Version(2, 1, 0);
                        }
                    }
                    else if (version.IsValid() == false) {
                        version.Default();
                    }

                    // Get / Set AppInformation
                    if (request.Verb == Web::Request::HTTP_GET) {
                        // We are at the end.. this is getting App info
                        TRACE(Trace::Information, (_T("Serving the Application [%s] Description File"), selectedApp->second.Name().c_str()));

                        Core::ProxyType<Web::TextBody> info(_textBodies.Element());
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK");
                        result->ContentType = Web::MIME_TEXT_XML;
                        Core::ProxyType<Web::TextBody> textBody(_textBodies.Element());
                        selectedApp->second.GetData(*textBody, version);
                        result->Body(textBody);
                        TRACE(Protocol, (static_cast<const string&>(*textBody)));
                    } else if (request.Verb == Web::Request::HTTP_POST) {
                        StartApplication(request, result, selectedApp->second);
                    }
                    else if (request.Verb == Web::Request::HTTP_DELETE) {
                        StopApplication(request, result, selectedApp->second);
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
                        if (index.Next() == true && index.Current() == _HideCommand) {
                            if (selectedApp->second.IsRunning() == true) {
                                if (selectedApp->second.HasHide() == true) {
                                    result->ErrorCode = Web::STATUS_OK;
                                    result->Message = _T("OK");
                                    selectedApp->second.Hide();
                                } else {
                                    result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                                    result->Message = _T("Not Implemented");
                                }
                            } else {
                                result->ErrorCode = Web::STATUS_NOT_FOUND;
                                result->Message = _T("App not running");
                            }
                        } else {
                            StartApplication(request, result, selectedApp->second);
                        }
                    }
                } else if (index.Current() == _DefaultRunningExtension) {
                    if ((request.Verb == Web::Request::HTTP_POST) || (request.Verb == Web::Request::HTTP_DELETE)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK");
                        selectedApp->second.Running(request.Verb == Web::Request::HTTP_POST);
                    }
                } else if (index.Current() == ((DeprecatedAPI() == true) ? _T("Hidding") : _DefaultHiddenExtension)) {
                    if ((request.Verb == Web::Request::HTTP_POST) || (request.Verb == Web::Request::HTTP_DELETE)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK");
                        selectedApp->second.Hidden(request.Verb == Web::Request::HTTP_POST);
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

        if (request.Origin.IsSet() == true) {
            result->AccessControlOrigin = request.Origin.Value();
        }

        TRACE(Protocol, (&(*result)));
        return (result);
    }

    bool DIALServer::SafeOrigin(const Web::Request& request, const AppInformation& app) const {
        bool safe = true;

        if ((request.Origin.IsSet() == true) && (app.Origin().empty() == false)) {
            Core::OptionalType<string> hostPortion (Core::URL(request.Origin.Value().c_str()).Host());

            if (hostPortion.IsSet() == true) {
                Core::NodeId source(hostPortion.Value().c_str());

                safe = (source.IsLocalInterface() == true) || (hostPortion.Value().find(app.Origin()) != string::npos);
            }
        }

        return (safe);
    }

    void DIALServer::Activated(Exchange::IWebServer* pluginInterface)
    {
        string remote(_dialURL.Host().Value() + ':' + (_dialURL.Port().IsSet() ? Core::NumberType<uint16_t>(_dialURL.Port().Value()).Text() : _T("80")));

        _adminLock.Lock();

        // Let's set the URL of the WebServer, as it is active :-)
        _dialServiceImpl->Locator(pluginInterface->Accessor() + _dialPath);

        Core::URL url = Core::URL(pluginInterface->Accessor());
        if (url.Port().IsSet() == true) {
            _webServerPort = Core::NumberType<uint16_t>(url.Port().Value());
        }

        // Redirect all calls to the DIALServer, via a proxy.
        pluginInterface->AddProxy(_dialPath, _dialPath, remote);

        _adminLock.Unlock();
    }

    void DIALServer::Deactivated(Exchange::IWebServer* pluginInterface)
    {
        ASSERT(_dialServiceImpl != nullptr);

        _adminLock.Lock();

        _dialServiceImpl->Locator(_dialURL.Text());
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
