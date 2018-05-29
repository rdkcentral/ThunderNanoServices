#ifndef __PLUGINWEBSERVER_H
#define __PLUGINWEBSERVER_H

#include "Module.h"
#include <interfaces/IWebServer.h>
#include <interfaces/IMemory.h>

namespace WPEFramework {
namespace Plugin {

    class WebServer : public PluginHost::IPlugin {
    private:
        WebServer(const WebServer&) = delete;
        WebServer& operator=(const WebServer&) = delete;

        class Notification : public RPC::IRemoteProcess::INotification {
        private:
            Notification() = delete;
            Notification(const Notification&) = delete;

        public:
            explicit Notification(WebServer* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Notification()
            {
		TRACE_L1("WebServer::Notification destructed. Line: %d", __LINE__);
            }

        public:
            virtual void Activated(RPC::IRemoteProcess* /* process */) 
            {
            }
            virtual void Deactivated(RPC::IRemoteProcess* process)
            {
                _parent.Deactivated(process);
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
            END_INTERFACE_MAP

        private:
            WebServer& _parent;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , OutOfProcess(true)
            {
                Add(_T("outofprocess"), &OutOfProcess);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::Boolean OutOfProcess;
        };

    public:
		#ifdef __WIN32__ 
		#pragma warning( disable : 4355 )
		#endif
        WebServer()
            : _service(nullptr)
            , _server(nullptr)
            , _memory(nullptr)
            , _notification(this)
        {
        }
		#ifdef __WIN32__ 
		#pragma warning( default : 4355 )
		#endif
		virtual ~WebServer()
        {
        }

        BEGIN_INTERFACE_MAP(WebServer)
            INTERFACE_ENTRY(IPlugin)
            INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
            INTERFACE_AGGREGATE(Exchange::IWebServer, _server)
            INTERFACE_AGGREGATE(PluginHost::IStateControl, _server)
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

        // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const;

	private:
        void Deactivated(RPC::IRemoteProcess* process);

    private:
        uint32_t _skipURL;
        uint32_t _pid;
        PluginHost::IShell* _service;
        Exchange::IWebServer* _server;
        Exchange::IMemory* _memory;
        Core::Sink<Notification> _notification;
    };
}
}

#endif // __PLUGINWEBSERVER_H
