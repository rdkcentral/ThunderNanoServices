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

#ifndef __PLUGINDIALSERVER_H
#define __PLUGINDIALSERVER_H

#include "Module.h"
#include <interfaces/ISwitchBoard.h>
#include <interfaces/IWebServer.h>
#include <interfaces/IBrowser.h>

namespace WPEFramework {
namespace Plugin {

    class DIALServer : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        class Config : public Core::JSON::Container {
        public:
            class App : public Core::JSON::Container {
            private:
                App& operator=(const App&) = delete;

            public:
                App()
                    : Core::JSON::Container()
                    , Name()
                    , Callsign()
                    , Handler()
                    , URL()
                    , Config()
                    , RuntimeChange(false)
                    , Hide(false)
                {
                    Add(_T("name"), &Name);
                    Add(_T("callsign"), &Callsign);
                    Add(_T("handler"), &Handler);
                    Add(_T("url"), &URL);
                    Add(_T("config"), &Config);
                    Add(_T("runtimechange"), &RuntimeChange);
                    Add(_T("hide"), &Hide);
                }
                App(const App& copy)
                    : Core::JSON::Container()
                    , Name(copy.Name)
                    , Callsign(copy.Callsign)
                    , Handler(copy.Handler)
                    , URL(copy.URL)
                    , Config(copy.Config)
                    , RuntimeChange(copy.RuntimeChange)
                    , Hide(copy.Hide)
                {
                    Add(_T("name"), &Name);
                    Add(_T("callsign"), &Callsign);
                    Add(_T("handler"), &Handler);
                    Add(_T("url"), &URL);
                    Add(_T("config"), &Config);
                    Add(_T("runtimechange"), &RuntimeChange);
                    Add(_T("hide"), &Hide);
                }
                virtual ~App()
                {
                }

            public:
                Core::JSON::String Name;
                Core::JSON::String Callsign;
                Core::JSON::String Handler;
                Core::JSON::String URL;
                Core::JSON::String Config;
                Core::JSON::Boolean RuntimeChange;
                Core::JSON::Boolean Hide;
            };

        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Name()
                , Model()
                , Description()
                , ModelNumber()
                , ModelURL()
                , Manufacturer()
                , ManufacturerURL()
                , SerialNumber()
                , UPC()
                , Interface()
                , WebServer()
                , SwitchBoard()
            {
                Add(_T("interface"), &Interface);
                Add(_T("name"), &Name);
                Add(_T("model"), &Model);
                Add(_T("description"), &Description);
                Add(_T("modelnumber"), &ModelNumber);
                Add(_T("modelurl"), &ModelURL);
                Add(_T("manufacturer"), &Manufacturer);
                Add(_T("manufacturerurl"), &ManufacturerURL);
                Add(_T("serialnumber"), &SerialNumber);
                Add(_T("upc"), &UPC);
                Add(_T("webserver"), &WebServer);
                Add(_T("switchboard"), &SwitchBoard);
                Add(_T("apps"), &Apps);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Name;
            Core::JSON::String Model;
            Core::JSON::String Description;
            Core::JSON::String ModelNumber;
            Core::JSON::String ModelURL;
            Core::JSON::String Manufacturer;
            Core::JSON::String ManufacturerURL;
            Core::JSON::String SerialNumber;
            Core::JSON::String UPC;
            Core::JSON::String Interface;
            Core::JSON::String WebServer;
            Core::JSON::String SwitchBoard;
            Core::JSON::ArrayType<App> Apps;
        };

        struct IApplication {
            using AdditionalDataType = std::unordered_map<string, string>;
            virtual ~IApplication() {}

            virtual bool IsRunning() const = 0;

            // Returns wheter DIAL handler has ability to start & stop the service
            virtual bool HasStartAndStop() const = 0;

            // Returns wheter DIAL handler has ability to hide & show a service
            virtual bool HasHideAndShow() const = 0;

            // Start an application with specified URL / payload
            // Can only be called if HasStartAndStop() evaluates to true
            virtual uint32_t Start(const string& parameters, const string& payload) = 0;

            // Connect DIAL handler with the service (eg. DIAL of youtube to cobalt).
            // Returns true if connection is successfull, false otherwise
            virtual bool Connect() = 0;

            // Returns whether DIAL handler is connected with the service
            virtual bool IsConnected() = 0;

            // Stop a running service. Additional parameters can be passed if in passive mode
            // Can only be called if HasStartAndStop() evaluates to true
            virtual void Stop(const string& parameters, const string& payload) = 0;

            virtual bool IsHidden() const = 0;

            // Make serivce visible. Can be used only if HasHideAndShow() evaluates to true
            virtual uint32_t Show() = 0;

            // Hide service. Can be used only if HasHideAndShow() evaluates to true
            virtual void Hide() = 0;

            // Methods for passing a URL to DIAL handler
            virtual string URL() const = 0;
            virtual bool URL(const string& url, const string& payload) = 0;

            // Methods used for passing additional data to DIAL handler
            virtual AdditionalDataType AdditionalData() const = 0;
            virtual void AdditionalData(AdditionalDataType&& data) = 0;

            // Method used for setting the wheter managed service is running or not. 
            // Used only in passive mode
            virtual void Running(const bool isRunning) = 0;

            // Method used for setting the wheter managed service is hidden or not. 
            // Used only in passive mode
            virtual void Hidden(const bool isHidden) = 0;

            // Method used for passing a SwitchBoard to DIAL handler. 
            // Used only in switchboard mode
            virtual void SwitchBoard(Exchange::ISwitchBoard* switchBoard) = 0;
        };

        struct IApplicationFactory {
            virtual ~IApplicationFactory() {}

            virtual IApplication* Create(PluginHost::IShell* shell, const Config::App& config, DIALServer* parent) = 0;
        };

        // FIXME: For now this is a stub only but at some point it'll have to call
        // something which supports low power mode.
        struct System : public Plugin::DIALServer::IApplication {
            ~System() override {}
            bool IsRunning() const { return true; }
            bool HasStartAndStop() const override { return false; }
            uint32_t Start(const string& parameters, const string& payload) override {
                ASSERT(!"Not supported and not even supposed to");
                return Core::ERROR_GENERAL;
            }
            bool Connect() override { return true;}
            bool IsConnected() override {return true;}
            void Stop(const string& parameters, const string& payload) { ASSERT(!"Not supported and not even supposed to"); }
            bool HasHideAndShow() const { return true; }
            bool IsHidden() const { return true; }
            uint32_t Show() override { return Core::ERROR_GENERAL; }
            void Hide() override {}
            string URL() const override { return {}; }
            bool URL(const string& url, const string& payload) override { return (false); };
            AdditionalDataType AdditionalData() const override { return { }; }
            void AdditionalData(AdditionalDataType&& data) override {}
            void Running(const bool isRunning) override {}
            void Hidden(const bool isHidden) override {}
            void SwitchBoard(Exchange::ISwitchBoard* switchBoard) override {}
        };

        struct SystemApplicationFactory  : public IApplicationFactory{
            virtual ~SystemApplicationFactory() {}

            IApplication* Create(PluginHost::IShell* shell, const Config::App& config, DIALServer* parent) override {
                return new System;
            }
        };

        class Default : public Plugin::DIALServer::IApplication {
        private:
            Default() = delete;
            Default(const Default&) = delete;
            Default& operator=(const Default&) = delete;

        public:
            Default(PluginHost::IShell* service, const Plugin::DIALServer::Config::App& config, DIALServer* parent)
                : _switchBoard(nullptr)
                , _service(service)
                , _callsign(config.Callsign.IsSet() == true ? config.Callsign.Value() : config.Name.Value())
                , _passiveMode(config.Callsign.IsSet() == false)
                , _isRunning(false)
                , _isHidden(false)
                , _hasRuntimeChange(config.RuntimeChange.Value())
                , _hasHideAndShow(config.Hide.Value())
                , _parent(parent)
            {
                ASSERT(_parent != nullptr);

                // The switchboard should be located on the Controller. If the Switchboard is configured
                // it should always result in an non-null ptr.
                if (_passiveMode == false) {
                    // We are in active mode, no need to do reporting from DIALSserver, move
                    // to the required plugin
                    _service = _service->QueryInterfaceByCallsign<PluginHost::IShell>(_callsign);

                    if (_service == nullptr) {
                        // Oops the service we want to use does not exist, move to Passive more..
                        _passiveMode = true;
                        _service = service;
                    }
                }

                if (_passiveMode == true) {
                    // Just need an addRef on the Service
                    _service->AddRef();
                }
            }
            virtual ~Default()
            {
                if (_switchBoard != nullptr) {
                    _switchBoard->Release();
                }
                if (_service != nullptr) {
                    _service->Release();
                }
            }

        public:
            // Methods that the DIALServer requires.
            virtual bool IsRunning() const
            {
                return (_passiveMode == true ? _isRunning : (_switchBoard != nullptr ? _switchBoard->IsActive(_callsign) : (_service->State() == PluginHost::IShell::ACTIVATED)));
            }
            bool IsHidden() const override { return _isHidden; }
            bool HasHideAndShow() const override { return _hasHideAndShow; }
            bool HasStartAndStop() const override { return true; }
            uint32_t Show() override 
            {
                if ((_passiveMode == true) && (_isHidden ==true)) {
                    const string message(_T("{ \"application\": \"") + _callsign + _T("\", \"request\":\"show\" }"));
                    _service->Notify(message);
                    _parent->event_show(_callsign);   
                } 
                return Core::ERROR_NONE; 
            }
            void Hide() override 
            {
                if (_passiveMode == true) {
                    const string message(_T("{ \"application\": \"") + _callsign + _T("\", \"request\":\"hide\" }"));
                    _service->Notify(message);
                    _parent->event_hide(_callsign);
                }
            }
            virtual uint32_t Start(const string& parameters, const string& payload)
            {
                uint32_t result = Core::ERROR_NONE;
                if (_passiveMode == true) {
                    const string message(_T("{ \"application\": \"") + _callsign + _T("\", \"request\":\"start\",  \"parameters\":\"" + parameters +  ", \"payload\":\"" + payload +"\" }"));
                    _service->Notify(message);
                    _parent->event_start(_callsign, parameters, payload);
                    
                } else {
                    if (_switchBoard != nullptr) {
                        result = _switchBoard->Activate(_callsign);
                    } else {
                        result = _service->Activate(PluginHost::IShell::REQUESTED);
                    }

                    if (IsRunning() == true) {
                        if (Connect() == false) {
                            TRACE_L1("DIAL: Failed to attach to service");
                            result = Core::ERROR_UNAVAILABLE;
                        } else {
                            URL(parameters, payload);
                        }
                    }
                }

                return result;
            }
            virtual void Stop(const string& parameters, const string& payload)
            {
                if (_passiveMode == true) {
                    const string message(_T("{ \"application\": \"") + _callsign + _T("\", \"request\":\"stop\", \"parameters\":\"" + parameters + ", \"payload\":\"" + payload + "\"}"));            
                    _service->Notify(message);
                    _parent->event_stop(_callsign, parameters);
                } else {
                    if (_switchBoard != nullptr) {
                        _switchBoard->Deactivate(_callsign);
                    } else {
                        _service->Deactivate(PluginHost::IShell::REQUESTED);
                    }
                }
            }
            bool IsConnected() override
            {
                return true;
            }
            bool Connect() override
            {
                return true;
            }
            string URL() const override
            {
                return ("");
            }
            bool URL(const string& url, const string& payload) override
            {
                bool result = false;

                if (_hasRuntimeChange == true) {
                    if (_passiveMode == true) {
                        const string message(_T("{ \"application\": \"") + _callsign + _T("\", \"request\":\"change\", \"parameters\":\"" + url + "\"}"));
                        _service->Notify(message);
                        result = true;
                    }
                    else {
                        Exchange::IBrowser* browser = _service->QueryInterface<Exchange::IBrowser>();

                        if (browser != nullptr) {
                            browser->SetURL(url);
                            browser->Release();
                            result = true;
                        }
                    }
                }
                return (result);
            }
            void AdditionalData(AdditionalDataType&& data) override
            {
                _additionalData = std::move(data);
            }
            AdditionalDataType AdditionalData() const override
            {
                return _additionalData;
            }
            virtual void Running(const bool isRunning)
            {
                // This method is only for the Passive mode..
                if (_passiveMode != true) {
                    TRACE_L1(_T("This app is not configured to be Passive !!!!%s"), "");
                }

                _isRunning = isRunning;
            }
            virtual void Hidden(const bool isHidden)
            {
                // This method is only for the Passive mode..
                if (_passiveMode != true) {
                    TRACE_L1(_T("This app is not configured to be Passive !!!!%s"), "");
                }

                _isHidden = isHidden;
            }
            virtual void SwitchBoard(Exchange::ISwitchBoard* switchBoard)
            {
                ASSERT((_switchBoard != nullptr) ^ (switchBoard != nullptr));

                if (_switchBoard != nullptr) {
                    _switchBoard->Release();
                }
                _switchBoard = switchBoard;

                if (_switchBoard != nullptr) {
                    _switchBoard->AddRef();
                }
            }

        protected:
            template <typename REQUESTEDINTERFACE>
            REQUESTEDINTERFACE* QueryInterface()
            {
                return (_service->QueryInterface<REQUESTEDINTERFACE>());
            }

        private:
            Exchange::ISwitchBoard* _switchBoard;
            PluginHost::IShell* _service;
            string _callsign;
            bool _passiveMode;
            bool _isRunning;
            bool _isHidden;
            bool _hasRuntimeChange;
            bool _hasHideAndShow;
            DIALServer* _parent;
            AdditionalDataType _additionalData;
        };

    private:
        DIALServer(const DIALServer&) = delete;
        DIALServer& operator=(const DIALServer&) = delete;

        struct Version {
            Version(uint8_t major, uint8_t minor, uint8_t patch)
                : Major(major), Minor(minor), Patch(patch) {}
            Version() : Version(0, 0, 0) {}

            bool IsValid() const { return Major != 0 || Minor != 0 || Patch != 0; }

            bool IsDefault() const { return Major == kDefaultMajor && Minor == kDefaultMinor && Patch == kDefaultPatch; }

            bool operator<(const Version& other) const {
              bool result = false;
              if (other.Major > Major)
                  result = true;

              if (result == false && other.Major == Major) {
                  if (other.Minor > Minor)
                      result = true;

                  if (result == false && other.Minor == Minor) {
                      result = other.Patch > Patch;
                  }
              }

              return result;
            }

            bool operator==(const Version& other) const {
                return other.Major == Major && other.Minor == Minor && other.Patch == Patch;
            }

            bool operator!=(const Version& other) const {
                return !(*this == other);
            }

            bool operator<=(const Version& other) const {
                return *this < other || *this == other;
            }

            bool operator>(const Version& other) const {
                return !(*this <= other);
            }

            bool operator>=(const Version& other) const {
                return *this > other || *this == other;
            }

            void SetDefault() {
              Major = kDefaultMajor;
              Minor = kDefaultMinor;
              Patch = kDefaultPatch;
            }

            void Clear() { Major = Minor = Patch = 0; }

            uint8_t Major;
            uint8_t Minor;
            uint8_t Patch;

            static constexpr uint8_t kDefaultMajor = 1;
            static constexpr uint8_t kDefaultMinor = 7;
            static constexpr uint8_t kDefaultPatch = 2;
        };

        static const uint32_t MaxDialQuerySize = 4096;

        static void ParseVersion(const string& version, Version* parsed)
        {
            ASSERT(parsed);
            if (version.empty() == true) {
                parsed->SetDefault();
            } else {
                parsed->Clear();
                auto dotPos = version.find('.');
                if (dotPos == string::npos) {
                    parsed->Major = atoi(version.c_str());
                } else {
                    string majorString = { version.c_str(), dotPos };
                    parsed->Major = atoi(majorString.c_str());
                    auto prevDotPos = dotPos + 1;
                    dotPos = version.find('.', prevDotPos);
                     if (dotPos != string::npos) {
                        string minorString = { version.c_str() + prevDotPos, dotPos - prevDotPos };
                        parsed->Minor = atoi(minorString.c_str());
                        prevDotPos = dotPos + 1;
                        dotPos = version.find('.', prevDotPos);
                        if (dotPos == string::npos) {
                          dotPos = version.size();
                        }
                        if (dotPos > prevDotPos) {
                           string patchString = { version.c_str() + prevDotPos, dotPos - prevDotPos };
                           parsed->Patch = atoi(patchString.c_str());
                        }
                     } else {
                        string minorString = { version.c_str() + prevDotPos, version.length() - prevDotPos };
                        parsed->Minor = atoi(minorString.c_str());
                     }
                }
            }
        }

        template <typename HANDLER>
        class ApplicationFactoryType : public IApplicationFactory {
        private:
            ApplicationFactoryType(const ApplicationFactoryType&) = delete;
            ApplicationFactoryType& operator=(const ApplicationFactoryType&) = delete;

        public:
            ApplicationFactoryType() {}
            virtual ~ApplicationFactoryType() {}

        public:
            virtual IApplication* Create(PluginHost::IShell* shell, const Config::App& config, DIALServer* parent)
            {
                IApplication* application = nullptr;
                if (config.Callsign.IsSet() == true) {
                    return (new HANDLER(shell, config, parent));
                }
                return application;
            }
        };
        class Protocol {
        private:
            // -------------------------------------------------------------------
            // This object should not be copied or assigned. Prevent the copy
            // constructor and assignment constructor from being used. Compiler
            // generated assignment and copy methods will be blocked by the
            // following statments.
            // Define them but do not implement them, compile error/link error.
            // -------------------------------------------------------------------
            Protocol(const Protocol& a_Copy) = delete;
            Protocol& operator=(const Protocol& a_RHS) = delete;

        public:
            Protocol(const Web::Request* request)
            {
                _text = Core::ToString(string("IN: ") + Web::Request::ToString(request->Verb) + ' ' + request->Path);
                if (request->Query.IsSet() == true) {
                    _text += "?" + request->Query.Value();
                }
            }
            Protocol(const Web::Response* response)
            {
                _text = Core::ToString(string("OUT: ") + response->Location.Value()) + " STATUS: " + std::to_string(response->ErrorCode) + " Message: " + response->Message;
            }
            Protocol(const string& response)
            {
                _text = Core::ToString(string("OUT: ") + response);
            }
            Protocol(const string& response, const Core::URL* url)
            {
                _text = Core::ToString(string("OUT: [") + url->Text() + "] " + response);
            }
            ~Protocol()
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
        class WebTransform {
        private:
            WebTransform(const WebTransform&);
            WebTransform& operator=(const WebTransform&);

        public:
            inline WebTransform()
                : _keywordLength(static_cast<uint8_t>(_tcslen(Web::Request::MSEARCH)))
            {
            }
            inline ~WebTransform()
            {
            }

            // Methods to extract and insert data into the socket buffers
            uint16_t Transform(Web::Request::Deserializer& deserializer, uint8_t* dataFrame, const uint16_t maxSendSize);

        private:
            uint8_t _keywordLength;
        };
        class DIALServerImpl : public Web::WebLinkType<Core::SocketDatagram, Web::Request, Web::Response, Core::ProxyPoolType<Web::Request>, WebTransform> {
        private:
            static const Core::NodeId DialServerInterface;
            typedef Web::WebLinkType<Core::SocketDatagram, Web::Request, Web::Response, Core::ProxyPoolType<Web::Request>, WebTransform> BaseClass;

            DIALServerImpl(const DIALServerImpl&) = delete;
            DIALServerImpl& operator=(const DIALServerImpl&) = delete;

        public:
            DIALServerImpl(const string& MACAddress, const string& baseURL, const string& appPath);
            virtual ~DIALServerImpl();

        public:
            // Notification of a Partial Request received, time to attach a body..
            virtual void LinkBody(Core::ProxyType<Web::Request>& element);

            // Notification of a Request received.
            virtual void Received(Core::ProxyType<Web::Request>& text);

            // Notification of a Response send.
            virtual void Send(const Core::ProxyType<Web::Response>& text);

            // Notification of a channel state change..
            virtual void StateChange();

            inline string URL() const
            {
                string result;

                _lock.Lock();

                result = _baseURL + '/' + _appPath;

                _lock.Unlock();

                return (result);
            }
            inline void URL(Core::URL& locator) const
            {
                locator = Core::URL(URL());
            }
            inline void Locator(const string& hostName)
            {
                _lock.Lock();

                _baseURL = hostName;

                _lock.Unlock();
            }

        private:
            mutable Core::CriticalSection _lock;
            // This should be the "Response" as depicted by the parent/DIALserver.
            Core::ProxyType<Web::Response> _response;
            std::list<Core::NodeId> _destinations;
            string _baseURL;
            const string _appPath;
        };
        class AppInformation {
        private:
            AppInformation() = delete;
            AppInformation(const AppInformation&) = delete;
            AppInformation& operator=(const AppInformation&) = delete;

        public:
            AppInformation(PluginHost::IShell* service, const Config::App& info, DIALServer* parent)
                : _lock()
                , _name(info.Name.Value())
                , _url(info.URL.Value())
                , _application(nullptr)
            {
                ASSERT(parent != nullptr);

                if ((info.Handler.IsSet() == true) && (info.Handler.Value().empty() == false)) {
                    std::map<string, IApplicationFactory*>::iterator index(_applicationFactory.find(info.Handler.Value()));
                    if (index != _applicationFactory.end()) {
                        _application = index->second->Create(service, info, parent);
                    }
                }

                if (_application == nullptr) {
                    std::map<string, IApplicationFactory*>::iterator index(_applicationFactory.find(info.Name.Value()));
                    if (index != _applicationFactory.end()) {
                        _application = index->second->Create(service, info, parent);
                    }
                }

                if (_application == nullptr) {
                    // since we still have nothing, fall back to the default
                    _application = new DIALServer::Default(service, info, parent);
                }
            }
            ~AppInformation()
            {
                if (_application != nullptr) {
                    delete _application;
                }
            }

        public:
            inline const string& Name() const
            {
                return (_name);
            }
            inline const string& AppURL() const
            {
                return (_url);
            }
            
            inline bool IsRunning() const 
            { 
                return _application->IsRunning(); 
            }
            inline bool IsHidden() const 
            { 
                return (_application->IsHidden()); 
            }
            inline bool HasHideAndShow() const
            {
                return _application->HasHideAndShow();
            }
            inline uint32_t Show()
            {
                return _application->Show();
            }
            inline void Hide() 
            { 
                _application->Hide(); 
            }
            bool Connect() 
            {
                return _application->Connect();
            }
            bool IsConnected() 
            {
                return _application->IsConnected();
            }
            inline void Running(const bool isRunning)
            {
                _application->Running(isRunning);
            }
            inline void Hidden(const bool isHidden)
            {
                _application->Hidden(isHidden);
            }
            inline uint32_t Start(const string& parameters, const string& payload)
            {
                return _application->Start(parameters, payload);
            }
            inline void Stop(const string& parameters, const string& payload)
            {
                _application->Stop(parameters, payload);
            }
            inline bool HasStartAndStop() const
            {
                return (_application->HasStartAndStop());
            }
            inline IApplication::AdditionalDataType AdditionalData() const
            {
                return (_application->AdditionalData());
            }
            inline string URL() const
            {
                return (_application->URL());
            }
            inline bool URL(const string& url, const string& payload)
            {
                return (_application->URL(url, payload));
            }
            inline void SwitchBoard(Exchange::ISwitchBoard* switchBoard)
            {
                _application->SwitchBoard(switchBoard);
            }

            inline static void Announce(const string& name, IApplicationFactory* factory)
            {
                ASSERT(AppInformation::_applicationFactory.find(name) == AppInformation::_applicationFactory.end());

                AppInformation::_applicationFactory.insert(std::pair<string, IApplicationFactory*>(name, factory));
            }
            inline static void Revoke(const string& name)
            {

                std::map<string, IApplicationFactory*>::iterator index = AppInformation::_applicationFactory.find(name);

                ASSERT(index != AppInformation::_applicationFactory.end());

                delete index->second;

                AppInformation::_applicationFactory.erase(index);
            }

            inline bool HasQueryParameter()
            {
                return (_url.find('?') != string::npos);
            }

            void GetData(string& data, const Version& version = {}) const;
            void SetData(const string& data);

        private:
            string XMLEncode(const string& source) const
            {
                string result;
                uint32_t index = 0;
                while (index < source.length()) {
                    switch (source[index]) {
                    case '&':
                        result += _T("&amp;");
                        break;
                    case '\"':
                        result += _T("&quot;");
                        break;
                    case '\'':
                        result += _T("&apos;");
                        break;
                    case '<':
                        result += _T("&lt;");
                        break;
                    case '>':
                        result += _T("&gt;");
                        break;
                    default:
                        result += source[index];
                        break;
                    }
                    ++index;
                }

                return (result);
            }

        private:
            mutable Core::CriticalSection _lock;
            const string _name;
            const string _url;
            IApplication* _application;

            static std::map<string, IApplicationFactory*> _applicationFactory;
        };
        class Notification : public PluginHost::IPlugin::INotification {
        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            Notification(DIALServer* parent)
                : _parent(*parent)
                , _webServer()
                , _switchBoard()
                , _webServerPtr(nullptr)
                , _switchBoardPtr(nullptr)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            void Register(PluginHost::IShell* service, const string& webServer, const string& switchBoard)
            {
                // This method needs to be Unregistered, before you can Regsiter it..
                ASSERT((_webServer.empty() == true) && (_switchBoard.empty() == true));

                _webServer = webServer;
                _switchBoard = switchBoard;

                if ((_webServer.empty() == false) || (_switchBoard.empty() == false)) {
                    service->Register(this);
                }
            }

            void Unregister(PluginHost::IShell* service)
            {
                if ((_webServer.empty() == false) || (_switchBoard.empty() == false)) {
                    service->Unregister(this);

                    if (_webServerPtr != nullptr) {
                        _parent.Deactivated(_webServerPtr);
                        _webServerPtr->Release();
                        _webServerPtr = nullptr;
                    }
                    if (_switchBoardPtr != nullptr) {
                        _parent.Deactivated(_switchBoardPtr);
                        _switchBoardPtr->Release();
                        _switchBoardPtr = nullptr;
                    }
                }
            }

            BEGIN_INTERFACE_MAP(ThisClass)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            virtual void StateChange(PluginHost::IShell* shell)
            {
                if (shell->Callsign() == _webServer) {

                    if (shell->State() == PluginHost::IShell::ACTIVATED) {
                        ASSERT(_webServerPtr == nullptr);

                        _webServerPtr = shell->QueryInterface<Exchange::IWebServer>();

                        if (_webServerPtr != nullptr) {
                            _parent.Activated(_webServerPtr);
                        }
                    } else if (shell->State() == PluginHost::IShell::DEACTIVATION) {

                        if (_webServerPtr != nullptr) {
                            _parent.Deactivated(_webServerPtr);
                            _webServerPtr->Release();
                            _webServerPtr = nullptr;
                        }
                    }
                } else if (shell->Callsign() == _switchBoard) {

                    if (shell->State() == PluginHost::IShell::ACTIVATED) {

                        _switchBoardPtr = shell->QueryInterface<Exchange::ISwitchBoard>();

                        if (_switchBoardPtr != nullptr) {
                            _parent.Activated(_switchBoardPtr);
                        }
                    } else if (shell->State() == PluginHost::IShell::DEACTIVATION) {

                        if (_switchBoardPtr != nullptr) {
                            _parent.Deactivated(_switchBoardPtr);
                            _switchBoardPtr->Release();
                            _switchBoardPtr = nullptr;
                        }
                    }
                }
            }

        private:
            DIALServer& _parent;
            string _webServer;
            string _switchBoard;
            Exchange::IWebServer* _webServerPtr;
            Exchange::ISwitchBoard* _switchBoardPtr;
        };

    public:
        template <typename HANDLER>
        class ApplicationRegistrationType {
        private:
            ApplicationRegistrationType(const ApplicationRegistrationType<HANDLER>&) = delete;
            ApplicationRegistrationType& operator=(const ApplicationRegistrationType<HANDLER>&) = delete;

        public:
            ApplicationRegistrationType()
            {
                string name(Core::ClassNameOnly(typeid(HANDLER).name()).Text());

                AppInformation::Announce(name, new ApplicationFactoryType<HANDLER>());
            }
            virtual ~ApplicationRegistrationType()
            {
                string name(Core::ClassNameOnly(typeid(HANDLER).name()).Text());

                AppInformation::Revoke(name);
            }
        };

    public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        DIALServer()
            : _adminLock()
            , _config()
            , _service(NULL)
            , _dialURL()
            , _dialPath()
            , _dialServiceImpl(NULL)
            , _deviceInfo(Core::ProxyType<Web::TextBody>::Create())
            , _sink(this)
            , _appInfo()
        {
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
        virtual ~DIALServer()
        {
        }

        BEGIN_INTERFACE_MAP(DIALServer)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        virtual const string Initialize(PluginHost::IShell* service);

        // The plugin is unloaded from the framework. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const;

        //      IWeb methods
        // -------------------------------------------------------------------------------------------------------
        // Whenever a request is received, it might carry some additional data in the body. This method allows
        // the plugin to attach a deserializable data object (ref counted) to be loaded with any potential found
        // in the body of the request.
        virtual void Inbound(Web::Request& request);

        // If everything is received correctly, the request is passed on to us, through a thread from the thread pool, to
        // do our thing and to return the result in the response object. Here the actual specific module work,
        // based on a a request is handled.
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

    private:
        void Activated(Exchange::IWebServer* webserver);
        void Deactivated(Exchange::IWebServer* webserver);
        void Activated(Exchange::ISwitchBoard* switchBoard);
        void Deactivated(Exchange::ISwitchBoard* switchBoard);
        void StartApplication(const Web::Request& request, Core::ProxyType<Web::Response>& response, AppInformation& app);
        void StopApplication(const Web::Request& request, Core::ProxyType<Web::Response>& response, AppInformation& app);

        //JsonRpc
        void event_start(const string& application, const string& parameters, const string& payload);
        void event_stop(const string& application, const string& parameters);
        void event_hide(const string& application);
        void event_show(const string& application);

    private:
        Core::CriticalSection _adminLock;
        uint32_t _skipURL;
        Config _config;
        PluginHost::IShell* _service;
        Core::URL _dialURL;
        string _dialPath;
        DIALServerImpl* _dialServiceImpl;
        Core::ProxyType<Web::TextBody> _deviceInfo;
        Core::Sink<Notification> _sink;
        std::map<const string, AppInformation> _appInfo;
    };
}
}

#endif // __PLUGINDIALSERVER_H
