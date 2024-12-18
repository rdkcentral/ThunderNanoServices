/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "WifiControl.h"
#include <interfaces/IConfiguration.h>

namespace Thunder {

namespace Plugin
{

    using namespace JsonData::WifiControl;

    namespace {

        static Metadata<WifiControl> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::PLATFORM },
            // Terminations
            {},
            // Controls
            {}
        );
    }

    static Core::ProxyPoolType<Web::JSONBodyType<JsonData::WifiControl::StatusResultData>> jsonResponseFactoryStatus(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfoData>>> jsonResponseFactoryNetworkList(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<JsonData::WifiControl::SecurityInfoData>>> jsonResponseFactorySecurityList(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<Core::JSON::String>>> jsonResponseFactoryConfigList(1);
    static Core::ProxyPoolType<Web::JSONBodyType<JsonData::WifiControl::ConfigInfoInfo>> jsonResponseFactoryConfig(4);

    WifiControl::WifiControl()
        : _skipURL(0)
        , _connectionId(0)
        , _service(nullptr)
        , _wifiControl()
        , _connectionNotification(this)
        , _wifiNotification(this)
    {
    }

    /* virtual */ const string WifiControl::Initialize(PluginHost::IShell* service)
    {
        string message;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_wifiControl == nullptr);
        _service = service;
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        _service = service;
        _service->AddRef();

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_connectionNotification);

        Config config;
        config.FromString(service->ConfigLine());

        if (Core::Directory(service->PersistentPath().c_str()).CreatePath() == false) {
            message = Core::Format(_T("Config directory %s doesn't exist and could not be created!"), service->PersistentPath().c_str());
        } 
        else {
            _wifiControl = service->Root<Exchange::IWifiControl>(_connectionId, 2000, _T("WifiControlImplementation"));
            if (_wifiControl == nullptr) {
                message = _T("WifiControl could not be instantiated.");
            }
            else {
                _wifiControl->Register(&_wifiNotification);
                Exchange::JWifiControl::Register(*this, _wifiControl);

                Exchange::IConfiguration* configWifi = _wifiControl->QueryInterface<Exchange::IConfiguration>();
                ASSERT (configWifi != nullptr);

                if (configWifi != nullptr) {
                    if (configWifi->Configure(service) != Core::ERROR_NONE) {
                        message = _T("WifiControl could not be configured.");
                    }
                    configWifi->Release();
                }
            }
        }

        // On success return empty, to indicate there is no error text.
        return (message);
    }

    /* virtual */ void WifiControl::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        RPC::IRemoteConnection* connection = nullptr;

        ASSERT(_service == service);

        _service->Unregister(&_connectionNotification);

        if (_wifiControl != nullptr) {

            connection = _service->RemoteConnection(_connectionId);
            
            Exchange::JWifiControl::Unregister(*this);
            _wifiControl->Unregister(&_wifiNotification);

            VARIABLE_IS_NOT_USED uint32_t result = _wifiControl->Release();
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
            _wifiControl = nullptr;
        }


        // The connection can disappear in the meantime...
        if (connection != nullptr) {
            // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
            connection->Terminate();
            connection->Release();
        }

        _service->Release();
        _service = nullptr;
        _connectionId = 0;
    }

    /* virtual */ string WifiControl::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void WifiControl::Inbound(Web::Request & request)
    {
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL,
                                            static_cast<uint32_t>(request.Path.length() - _skipURL)),
            false, '/');

        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        if ((request.Verb == Web::Request::HTTP_PUT) || (request.Verb == Web::Request::HTTP_POST)) {
            if ((index.IsValid() == true) && (index.Next() == true) && (index.Current().Text() == _T("Config"))) {
                request.Body(jsonResponseFactoryConfig.Element());
            }
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> WifiControl::Process(const Web::Request& request)
    {
        TRACE(Trace::Information, (_T("Web request %s"), request.Path.c_str()));
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');
        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();
        if (request.Verb == Web::Request::HTTP_GET) {

            result = GetMethod(index);
        } else if (request.Verb == Web::Request::HTTP_PUT) {

            result = PutMethod(index);
        } else if (request.Verb == Web::Request::HTTP_POST) {

            result = PostMethod(index, request);
        } else if (request.Verb == Web::Request::HTTP_DELETE) {

            result = DeleteMethod(index);
        }

        return result;
    }

    inline void UpdateConfig(JsonData::WifiControl::ConfigInfoInfo& config, const Exchange::IWifiControl::ConfigInfo& configInfo)
    {
        config.Hidden = configInfo.hidden;
        config.Accesspoint = configInfo.accesspoint;
        config.Ssid = configInfo.ssid;
        config.Secret = configInfo.secret;
        config.Identity = configInfo.identity;
        config.Method = configInfo.method;
    }

    Core::ProxyType<Web::Response> WifiControl::GetMethod(Core::TextSegmentIterator & index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");
        uint32_t status = Core::ERROR_GENERAL;

        if (index.IsValid() == true) {
            if ((index.Next() == true) && (index.IsValid() == true)) {
                if (index.Current().Text() == _T("Networks")) {

                    Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfoData>>> networks(jsonResponseFactoryNetworkList.Element());
                    RPC::IIteratorType<Exchange::IWifiControl::NetworkInfo, Exchange::ID_WIFICONTROL_NETWORK_INFO_ITERATOR>* networkList;
                    status = _wifiControl->Networks(networkList);
                    if (status == Core::ERROR_NONE) {
                        if (networkList != nullptr) {
                            Exchange::IWifiControl::NetworkInfo networkInfo;
                            while (networkList->Next(networkInfo) == true) {
                                JsonData::WifiControl::NetworkInfoData& networkInfoData(networks->Add());
                                networkInfoData = networkInfo;
                            }
                            networkList->Release();
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Scanned networks.");
                            result->Body(networks);
                        }
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Empty networks.");
                    }

                } else if ((index.Current().Text() == _T("Securities")) && (index.Next() == true)) {

                    Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<JsonData::WifiControl::SecurityInfoData>>> securities(jsonResponseFactorySecurityList.Element());
                    RPC::IIteratorType<Exchange::IWifiControl::SecurityInfo, Exchange::ID_WIFICONTROL_SECURITY_INFO_ITERATOR>* securityList;
                    status = _wifiControl->Securities(index.Current().Text(), securityList);
                    if (status == Core::ERROR_NONE) {
                        if (securityList != nullptr) {
                            Exchange::IWifiControl::SecurityInfo securityInfo;
                            while (securityList->Next(securityInfo) == true) {
                                JsonData::WifiControl::SecurityInfoData& securityInfoData(securities->Add());
                                securityInfoData = securityInfo;
                            }
                            securityList->Release();
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Security info against networks: ") + index.Current().Text();
                            result->Body(securities);
                        }
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Empty networks.");
                    }

                } else if (index.Current().Text() == _T("Configs")) {

                    Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<Core::JSON::String>>> configs(jsonResponseFactoryConfigList.Element());

                    RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>* configList;
                    status = _wifiControl->Configs(configList);
                    if (status == Core::ERROR_NONE) {
                        if (configList != nullptr) {
                            string config;
                            while (configList->Next(config) == true) {
                                Core::JSON::String& ssid(configs->Add());
                                ssid = config;
                            }
                            configList->Release();
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Get configurations.");
                            result->Body(configs);
                        }
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Empty configs.");
                    }
                } else if ((index.Current().Text() == _T("Config")) && (index.Next() == true)) {

                    Core::ProxyType<Web::JSONBodyType<JsonData::WifiControl::ConfigInfoInfo>> config(jsonResponseFactoryConfig.Element());

                    Exchange::IWifiControl::ConfigInfo configInfo;
                    status = (static_cast<const Exchange::IWifiControl*>(_wifiControl))->Config(index.Current().Text(), configInfo);
                    if (status == Core::ERROR_NONE) {
                        UpdateConfig(*config, configInfo);
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Get configuration.");
                        result->Body(config);
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Empty config.");
                    }
                }
            }
        } else {
            Core::ProxyType<Web::JSONBodyType<JsonData::WifiControl::StatusResultData>> statusResult(jsonResponseFactoryStatus.Element());

            string connectedSsid;
            bool isScanning;
            status = _wifiControl->Status(connectedSsid, isScanning);
            if (status == Core::ERROR_NONE) {
                statusResult->ConnectedSsid = connectedSsid;
                statusResult->IsScanning = isScanning;
                result->ErrorCode = Web::STATUS_OK;
                result->Message = _T("Current status.");
                result->Body(statusResult);
            } else {
                result->ErrorCode = Web::STATUS_NO_CONTENT;
                result->Message = _T("Status not available");
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> WifiControl::SetConfig(const Web::Request& request, const string& ssid)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::ProxyType<const JsonData::WifiControl::ConfigInfoInfo> config(request.Body<const JsonData::WifiControl::ConfigInfoInfo>());
        if (config.IsValid() == false) {
            result->ErrorCode = Web::STATUS_NO_CONTENT;
            result->Message = _T("Nothing to set in the config.");
        } else {

            uint32_t status = _wifiControl->Config(ssid, *config);

            if (status == Core::ERROR_NONE) {
                result->ErrorCode = Web::STATUS_OK;
                result->Message = _T("Config set.");
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Incomplete Config.");
            }
        }
        return result;
    }

    Core::ProxyType<Web::Response> WifiControl::PutMethod(Core::TextSegmentIterator & index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported PUT request.");
        uint32_t status = Core::ERROR_GENERAL;

        if (index.IsValid() == true) {
            if (index.Next()) {
                if (index.Current().Text() == _T("Scan")) {

                    result->ErrorCode = Web::STATUS_OK;
                    _wifiControl->Scan();
                    if (status != Core::ERROR_NONE) {
                        result->Message = _T("Failure in Scan");
                    } else {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Scan started.");
                    }

                } else if (index.Current().Text() == _T("AbortScan")) {

                    _wifiControl->AbortScan();

                    if (status != Core::ERROR_NONE) {
                        result->Message = _T("Failure in Scan");
                    } else {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Scan");
                    }

                } else if (index.Current().Text() == _T("Connect")) {
                    if (index.Next() == true) {

                        status = _wifiControl->Connect(index.Current().Text());
                        if (status != Core::ERROR_NONE) {
                            result->Message = _T("Failure in connect");
                        } else {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Connect started.");
                        }
                    }
                } else if (index.Current().Text() == _T("Disconnect")) {
                    if (index.Next() == true) {
                        status = _wifiControl->Disconnect(index.Current().Text());
                        if (status != Core::ERROR_NONE) {
                            result->Message = _T("Failure in disconnect");
                        } else {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Disconnected.");
                        }
                    }
                }
            }
        }
        return result;
    }

    Core::ProxyType<Web::Response> WifiControl::PostMethod(Core::TextSegmentIterator & index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST requestservice.");

        if ((index.IsValid() == true) && (index.Next() && (index.Current().Text() == _T("Config")))) {
            if (index.Next() == true) {
                result = SetConfig(request, index.Current().Text());
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> WifiControl::DeleteMethod(Core::TextSegmentIterator & index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported DELETE request.");

        if (index.IsValid() == true) {
            if (index.Next()) {
                if (index.Current().Text() == _T("Connect")) {
                    if (index.Next() == true) {
                        if (_wifiControl->Disconnect(index.Current().Text()) == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                             result->Message = _T("Disconnected.");
                        } else {
                            result->Message = _T("Failure in disconnect");
                        }
                    }
                }
            }
        }

        return result;
    }

    void WifiControl::Deactivated(RPC::IRemoteConnection* connection)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_connectionId == connection->Id()) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
} // namespace Plugin
} // namespace Thunder
