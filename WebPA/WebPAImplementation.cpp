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
 
#include "Module.h"
#include <interfaces/IWebPA.h>
#include <signal.h>
#include <string>

#ifdef __cplusplus
extern "C"
{
#endif
int ParodusServiceMain(int argc, char** argv);
#ifdef __cplusplus
}
#endif

namespace WPEFramework {

class WebPAImplementation :
    public Exchange::IWebPA, public WPEFramework::Core::Thread {

private:
    class Config : public Core::JSON::Container {
    public:
        Config(const Config&);
        Config& operator=(const Config&);

        Config()
            : Core::JSON::Container()
            , Interface(_T(""))
            , PingWaitTimeOut(_T(""))
            , WebPAUrl(_T(""))
            , ParodusLocalUrl(_T(""))
            , PartnerId(_T(""))
            , WebPABackOffMax(_T(""))
            , SSLCertPath(_T(""))
            , ForceIPV4(false)
            , Location()
        {
            Add(_T("interface"), &Interface);
            Add(_T("pingwaittime"), &PingWaitTimeOut);
            Add(_T("webpaurl"), &WebPAUrl);
            Add(_T("paroduslocalurl"), &ParodusLocalUrl);
            Add(_T("partnerid"), &PartnerId);
            Add(_T("webpabackoffmax"), &WebPABackOffMax);
            Add(_T("sslcertpath"), &SSLCertPath);
            Add(_T("forceipv4"), &ForceIPV4);
            Add(_T("location"), &Location);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::String Interface;
        Core::JSON::String PingWaitTimeOut;
        Core::JSON::String WebPAUrl;
        Core::JSON::String ParodusLocalUrl;
        Core::JSON::String PartnerId;
        Core::JSON::String WebPABackOffMax;
        Core::JSON::String SSLCertPath;
        Core::JSON::Boolean ForceIPV4;
        Core::JSON::String Location;
    };

private:
    static constexpr const TCHAR* NullMACAddress = _T("00:00:00:00:00:00");

public:
    WebPAImplementation(const WebPAImplementation&) = delete;
    WebPAImplementation& operator=(const WebPAImplementation&) = delete;

    WebPAImplementation ()
        : Core::Thread(0, _T("WebPAService"))
        , _options("WebPAService")
        , _adminLock()
    {
        TRACE(Trace::Information, (_T("WebPAImplementation::Construct()")));
    }

    virtual ~WebPAImplementation()
    {
        TRACE(Trace::Information, (_T("WebPAImplementation::Destruct()")));

        if (true == IsRunning()) {
            Signal(SIGINT);

            Stop();
            Wait(Thread::STOPPED, Core::infinite);
        }
    }

    BEGIN_INTERFACE_MAP(WebPAImplementation)
       INTERFACE_ENTRY(Exchange::IWebPA)
    END_INTERFACE_MAP

    //WebPAService Interface
    virtual uint32_t Initialize(PluginHost::IShell* service) override
    {
        ASSERT(nullptr != service);

        uint32_t result = Core::ERROR_NONE;

        Config config;
        config.FromString(service->ConfigLine());

        Core::Process::Options webpaService(_T("WebPAService"));

        // Get the point of entry on WPEFramework..
        Core::AdapterIterator interface;
        if (config.Interface.Value().empty() == false) {
            Core::AdapterIterator index = Core::AdapterIterator(config.Interface.Value());
            if (IsValidInterface(index) == true) {
                interface = index;
            }
        } else {
            while (interface.Next() == true) {
                if (IsValidInterface(interface) == true) {
                    break;
                }
            }
        }

        if (interface.IsValid() == true) {
            _options.Add(_T("-d")).Add(interface.MACAddress(':'));
            _options.Add(_T("-i")).Add(interface.Name());
        } else {
            TRACE(Trace::Error, (_T("WebPA: Error in configuring network interface for communication")));
        }
        if (config.PingWaitTimeOut.Value().empty() == false) {
            _options.Add(_T("-t")).Add(config.PingWaitTimeOut.Value());
        }
        if (config.WebPAUrl.Value().empty() == false) {
           _options.Add(_T("-u")).Add(config.WebPAUrl.Value());
        }
        if (config.ParodusLocalUrl.Value().empty() == false) {
            _options.Add(_T("-l")).Add(config.ParodusLocalUrl.Value());
        }
        if (config.PartnerId.Value().empty() == false) {
            _options.Add(_T("-p")).Add(config.PartnerId.Value());
        }
        if (config.WebPABackOffMax.Value().empty() == false) {
            _options.Add(_T("-o")).Add(config.WebPABackOffMax.Value());
        }
        if (config.SSLCertPath.Value().empty() == false) {
            _options.Add(_T("-c")).Add(config.SSLCertPath.Value());
        }
        if (config.ForceIPV4.IsSet() == true) {
            _options.Add(_T("4"));
        }
       const string locator(service->DataPath() + config.Location.Value());
       Launch();

       // Before we start loading the mapping of the Keys to the factories, load the factories :-)
       Core::Directory entry(locator.c_str(), _T("*.webpa"));
       Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());
       while (entry.Next() == true) {
           const TCHAR* className(Core::File::FileName(entry.Current()).c_str());
           uint32_t version(static_cast<uint32_t>(~0));

           _adminLock.Lock();
           Exchange::IWebPA::IWebPAClient* client = admin.Instantiate<Exchange::IWebPA::IWebPAClient>(entry.Current().c_str(), className, version);
           if (client != nullptr) {
               client->Configure(service);
               if (client->Launch() == Core::ERROR_NONE) {
                   _clients.insert(std::pair<const string, Exchange::IWebPA::IWebPAClient*>(className, client));
               } else {
                   client->Release();
                   TRACE(Trace::Error, (_T("WebPA Error in launching client %s"), entry.Current().c_str()));
               }
           } else {
               TRACE(Trace::Error, (_T("WebPA Error in the instantiation of client %s"), entry.Current().c_str()));
           }
           _adminLock.Unlock();
       }

       _adminLock.Lock();
       uint32_t clientSize = _clients.size();
       _adminLock.Unlock();

       if (clientSize == 0) {
            SYSLOG(Logging::Startup, (_T("No WebPA Clients registered")));
            result = Core::ERROR_UNAVAILABLE;
       }

       return result;
    }

    void Launch()
    {
        if (true == IsRunning())
            Signal(SIGINT);

        Block();
        Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
        if (State() == Thread::BLOCKED) {
            Run();
        }
    }

    virtual void Deinitialize(PluginHost::IShell* service) {

        _adminLock.Lock();
        // Stop processing of the webpa:
        for (auto client: _clients) {
            client.second->Release();
        }
        _clients.clear();
        _adminLock.Unlock();
    }

    virtual Exchange::IWebPA::IWebPAClient* Client(const std::string& name) override {

        Exchange::IWebPA::IWebPAClient* webpaClient = nullptr;

        _adminLock.Lock();
        std::map<const string, Exchange::IWebPA::IWebPAClient*>::iterator client(_clients.find(name));
        if (client == _clients.end()) {
            TRACE(Trace::Error, (_T("WebPA: could not able to find out client %s"), name.c_str()));
        } else {
            webpaClient = client->second;
        }
        _adminLock.Unlock();

        return webpaClient;
    }

private:
    virtual uint32_t Worker()
    {
        TRACE(Trace::Information, (string(__FUNCTION__)));
        uint16_t size = _options.BlockSize();
        void* parameters = ::malloc(size);

        int argc = _options.Block(parameters, size);
        TRACE(Trace::Information, (_T("Calling ParodusServiceMain()")));
        ParodusServiceMain(argc, &(reinterpret_cast<char**>(parameters)[0]));
    
        ::free(parameters);
        TRACE(Trace::Information, (_T("End of Worker\n")));
        return WPEFramework::Core::infinite;
    }

    inline bool IsValidInterface(Core::AdapterIterator interface) {
        return ((interface.IsValid() == true) && interface.IsUp() && (interface.MACAddress(':') != string(NullMACAddress)));
    }
private:
    Core::Process::Options _options;
    Core::CriticalSection _adminLock;
    std::map<const std::string, IWebPAClient*> _clients;
};

// The essence of making the IWebPAService interface available. This instantiates
// an object that can be created from the outside of the library by looking
// for the WebPAImplementation class name, that realizes the IStateControl interface.
SERVICE_REGISTRATION(WebPAImplementation, 1, 0);

}
