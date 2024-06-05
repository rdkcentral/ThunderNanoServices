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

#include "NetworkControl.h"
#include <interfaces/IConfiguration.h>

namespace Thunder {

namespace Plugin
{

    namespace {

        static Metadata<NetworkControl> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            { subsystem::NETWORK }
        );
    }

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<NetworkControl::StatusData>> jsonStatusFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<JsonData::NetworkControl::UpData>> jsonUpDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<Core::JSON::String>>> jsonStringListFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<JsonData::NetworkControl::NetworkInfoInfo>>> jsonNetworksFactory(1);

    NetworkControl::NetworkControl()
        : _skipURL(0)
        , _connectionId(0)
        , _service(nullptr)
        , _networkControl()
        , _connectionNotification(*this)
        , _networkNotification(*this)
    {
    }

    /* virtual */ const string NetworkControl::Initialize(PluginHost::IShell * service)
    {
        string message;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_networkControl == nullptr);
        _service = service;
        _service->AddRef();

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_connectionNotification);

        _networkControl = service->Root<Exchange::INetworkControl>(_connectionId, 2000, _T("NetworkControlImplementation"));
        if (_networkControl != nullptr) {
            _networkControl->Register(&_networkNotification);
            Exchange::JNetworkControl::Register(*this, _networkControl);

            Exchange::IConfiguration* configNetwork = _networkControl->QueryInterface<Exchange::IConfiguration>();
            if (configNetwork != nullptr) {
                configNetwork->Configure(service);
                configNetwork->Release();
            }
        }
        else {
            message = _T("NetworkControl could not be instantiated.");
        }

        // On success return empty, to indicate there is no error text.
        return (message);
    }

    /* virtual */ void NetworkControl::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
	    ASSERT(_service == service);

            _service->Unregister(&_connectionNotification);

            if (_networkControl != nullptr) {
                Exchange::JNetworkControl::Unregister(*this);
                _networkControl->Unregister(&_networkNotification);

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
                VARIABLE_IS_NOT_USED uint32_t result = _networkControl->Release();
                _networkControl = nullptr;
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // The connection can disappear in the meantime...
                if (connection != nullptr) {
                    // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                    connection->Terminate();
                    connection->Release();
                }
            }

            _service->Release();
            _service = nullptr;
            _connectionId = 0;
        }
    }

    /* virtual */ string NetworkControl::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void NetworkControl::Inbound(Web::Request& request)
    {
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL,
                                        static_cast<uint32_t>(request.Path.length() - _skipURL)),
                                        false, '/');

        // If it is a put, we expect a list of commands, receive them !!!!
        if (request.Verb == Web::Request::HTTP_POST) {
            index.Next();

            if ((index.IsValid() == true) && (index.Next() == true)) {
                if (index.Current().Text() == _T("DNS")) {
                    request.Body(jsonStringListFactory.Element());
                } else if (index.Current().Text() == _T("Network")) {
                    request.Body(jsonNetworksFactory.Element());
                }
            }
        }
    }

    /* virtual */ Core::ProxyType<Web::Response>
    NetworkControl::Process(const Web::Request& request)
    {

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint16_t>(request.Path.length()) - _skipURL), false, '/');

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [NetworkControl] service.");

        // By default, we skip the first slash
        index.Next();

        if (request.Verb == Web::Request::HTTP_GET) {
            result = GetMethod(index);
        } else if (request.Verb == Web::Request::HTTP_PUT) {
            result = PutMethod(index);
        } else if (request.Verb == Web::Request::HTTP_POST) {
            result = PostMethod(index, request);
        }
        return result;
    }

    Core::ProxyType<Web::Response> NetworkControl::GetMethod(Core::TextSegmentIterator& index) const
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");
        uint32_t status = Core::ERROR_GENERAL;

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                if ((index.Current().Text() == _T("Network")) && (index.Next() == true)) {

                    Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<JsonData::NetworkControl::NetworkInfoInfo>>> networks(jsonNetworksFactory.Element());
                    RPC::IIteratorType<Exchange::INetworkControl::NetworkInfo, Exchange::ID_NETWORKCONTROL_NETWORK_INFO_ITERATOR>* networkList;
                    status = static_cast<const Exchange::INetworkControl*>(_networkControl)->Network(index.Current().Text(), networkList);
                    if (status == Core::ERROR_NONE) {
                        if (networkList != nullptr) {
                            Exchange::INetworkControl::NetworkInfo networkInfo;
                            while (networkList->Next(networkInfo) == true) {
                                JsonData::NetworkControl::NetworkInfoInfo& networkInfoInfo(networks->Add());
                                networkInfoInfo = networkInfo;
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
                } else if (index.Current().Text() == _T("Interfaces")) {

                    Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<Core::JSON::String>>> interfaces(jsonStringListFactory.Element());

                    RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>* interfaceList;
                    status = _networkControl->Interfaces(interfaceList);
                    if (status == Core::ERROR_NONE) {
                        if (interfaceList != nullptr) {
                            string entry;
                            while (interfaceList->Next(entry) == true) {
                                Core::JSON::String& interface(interfaces->Add());
                                interface = entry;
                            }
                            interfaceList->Release();
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Get interfaces.");
                            result->Body(interfaces);
                        }
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Empty interfaces.");
                    }
                } else if (index.Current().Text() == _T("DNS")) {

                    Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<Core::JSON::String>>> dns(jsonStringListFactory.Element());

                    RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>* dnsList;
                    status = (static_cast<const Exchange::INetworkControl*>(_networkControl))->DNS(dnsList);
                    if (status == Core::ERROR_NONE) {
                        if (dnsList != nullptr) {
                            string entry;
                            while (dnsList->Next(entry) == true) {
                                Core::JSON::String& jsonDNS(dns->Add());
                                jsonDNS = entry;
                            }
                            dnsList->Release();
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Get dns.");
                            result->Body(dns);
                        }
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Empty dns.");
                    }

                } else if ((index.Current().Text() == _T("Status")) && (index.Next() == true)) {
                    Core::ProxyType<Web::JSONBodyType<NetworkControl::StatusData>> statusResult(jsonStatusFactory.Element());
                    Exchange::INetworkControl::StatusType networkStatus;
                    status = _networkControl->Status(index.Current().Text(), networkStatus);
                    if (status == Core::ERROR_NONE) {
                        statusResult->StatusType = networkStatus;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Status of interface ") + index.Current().Text();
                        result->Body(statusResult);
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Status not available");
                    }
                } else if ((index.Current().Text() == _T("Up")) && (index.Next() == true)) {
                    bool isUp;
                    status = static_cast<const Exchange::INetworkControl*>(_networkControl)->Up(index.Current().Text(), isUp);
                    Core::ProxyType<Web::JSONBodyType<JsonData::NetworkControl::UpData>> upData(jsonUpDataFactory.Element());
                    upData->Value = isUp;

                    if (status == Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(upData);
                        if (isUp) {
                            result->Message = _T("Interface is up");

                        } else {
                            result->Message = _T("Interface is down");
                        }
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Status not available");
                   }
                }
            }
        }
        return result;
    }

    Core::ProxyType<Web::Response> NetworkControl::PutMethod(Core::TextSegmentIterator& index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported PUT request.");
        uint32_t status = Core::ERROR_GENERAL;

        if ((index.IsValid() == true) && (index.Next() == true)) {
            if ((index.Current() == _T("Up")) && (index.Next() == true)) {
                string interface = index.Current().Text();
                status = _networkControl->Up(interface, true);

                if (status == Core::ERROR_NONE) {
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = string(_T("OK, ")) + interface + _T(" set UP.");
                } else {
                    result->Message = string(_T("Error in ")) + interface + _T(" set UP.");
                }
            } else if ((index.Current() == _T("Down")) && (index.Next() == true)) {
                string interface = index.Current().Text();
                status = _networkControl->Up(interface, false);

                if (status == Core::ERROR_NONE) {
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = string(_T("OK, ")) + interface + _T(" set DOWN.");
                } else {
                    result->Message = string(_T("Error in ")) + interface + _T(" set DOWN.");
                }
            } else if ((index.Current() == _T("Flush")) && (index.Next() == true)) {
                string interface = index.Current().Text();
                status = _networkControl->Flush(interface);
                if (status == Core::ERROR_NONE) {
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = string(_T("OK, ")) + interface + _T(" Flush.");
                } else {
                    result->Message = string(_T("Error in ")) + interface + _T(" Flush.");
                }
            }
        }
        return result;
    }

    Core::ProxyType<Web::Response> NetworkControl::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST request.");
        uint32_t status = Core::ERROR_GENERAL;

        if (index.IsValid() == true && (index.Next() == true)) {
            if ((index.Current().Text() == _T("DNS")) && (request.HasBody() == true)) {
                auto dns = request.Body<const Core::JSON::ArrayType<Core::JSON::String>>()->Elements();

                if (dns.Count()) {
                      std::list<string> elements;
                    while (dns.Next() == true) {
                        elements.push_back(dns.Current());
                    }
                    RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>* dnsList{Core::ServiceType<RPC::IteratorType<RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>>>::Create<RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>>(elements)};

                    status = _networkControl->DNS(static_cast<Exchange::INetworkControl::IStringIterator* const&>(dnsList));
                    dnsList->Release();
                    if (status == Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set dns.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Error to set dns.");
                    }
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = _T("Empty dns list.");
                }
            } else if ((index.Current().Text() == _T("Network")) && (index.Next() == true)) {
                string interface(index.Current().Text());
                auto network = request.Body<const Core::JSON::ArrayType<JsonData::NetworkControl::NetworkInfoInfo>>()->Elements();

                if (network.Count()) {
                    std::list<Exchange::INetworkControl::NetworkInfo> elements;
                    while (network.Next() == true) {
                        elements.push_back(network.Current());
                    }
                    RPC::IIteratorType<Exchange::INetworkControl::NetworkInfo, Exchange::ID_NETWORKCONTROL_NETWORK_INFO_ITERATOR>* networkList{Core::ServiceType<RPC::IteratorType<RPC::IIteratorType<Exchange::INetworkControl::NetworkInfo, Exchange::ID_NETWORKCONTROL_NETWORK_INFO_ITERATOR>>>::Create<RPC::IIteratorType<Exchange::INetworkControl::NetworkInfo, Exchange::ID_NETWORKCONTROL_NETWORK_INFO_ITERATOR>>(elements)};

                    status = _networkControl->Network(interface, static_cast<Exchange::INetworkControl::INetworkInfoIterator* const&>(networkList));
                    networkList->Release();
                    if (status == Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set network.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Error to set dns.");
                    }
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = _T("Empty network list.");
                }
            }
        }
        return result;
    }

    void NetworkControl::Deactivated(RPC::IRemoteConnection* connection)
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

