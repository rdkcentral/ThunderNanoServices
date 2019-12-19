#ifndef __DSGCCCLIENT_H
#define __DSGCCCLIENT_H

#include "Module.h"
#include <interfaces/IDsgccClient.h>
#include "DsgParser.h"

namespace WPEFramework {
namespace Plugin {
//using namespace Dsg;

    class DsgccClient : public PluginHost::IPlugin, public PluginHost::IWeb {
    private:
        DsgccClient(const DsgccClient&) = delete;
        DsgccClient& operator=(const DsgccClient&) = delete;

        class Sink : public Exchange::IDsgccClient::INotification {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

        public:
            Sink(DsgccClient* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Sink()
            {
            }

        public:
            void StateChange(const Exchange::IDsgccClient::state state) override
            {
                _parent.StateChange(state);
            }

            BEGIN_INTERFACE_MAP(Sink)
            INTERFACE_ENTRY(Exchange::IDsgccClient::INotification)
            END_INTERFACE_MAP

        private:
            DsgccClient& _parent;
        }; // Sink

        class Notification : public RPC::IRemoteConnection::INotification {

        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(DsgccClient* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            virtual void Activated(RPC::IRemoteConnection*)
            {
            }
            virtual void Deactivated(RPC::IRemoteConnection* connection)
            {
                _parent.Deactivated(connection);
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            DsgccClient& _parent;
        }; // Notification

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
        };  // Config

    public:
        class Data : public Core::JSON::Container {
        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , Str()
            {
                Add(_T("channels"), &Channels);
                Add(_T("State"), &State);
                Add(_T("Str"), &Str);
            }
            ~Data()
            {
            }

        public:
            Core::JSON::ArrayType<Channel> Channels;
            Core::JSON::String State;
            Core::JSON::String Str;
        }; //Data

    public:
        DsgccClient()
            : _service(nullptr)
            , _implementation(nullptr)
            , _notification(this)
            , _sink(this)
        {
        }

        virtual ~DsgccClient()
        {
        }

    public:
        BEGIN_INTERFACE_MAP(DsgccClient)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::IDsgccClient, _implementation)
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

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

    private:
        void Deactivated(RPC::IRemoteConnection* connection);
        void StateChange(Exchange::IDsgccClient::state state);

    private:
        uint8_t _skipURL;
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Exchange::IDsgccClient* _implementation;
        Core::Sink<Notification> _notification;
        Core::Sink<Sink> _sink;
    };

} //namespace Plugin
} //namespace WPEFramework

#endif // __DSGCCCLIENT_H
