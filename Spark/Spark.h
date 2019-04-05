#pragma once

#include "Module.h"
#include <interfaces/IBrowser.h>
#include <interfaces/IMemory.h>

namespace WPEFramework {
namespace Plugin {

    class Spark : public PluginHost::IPlugin, public PluginHost::IWeb /*, public PluginHost::JSONRPC*/  {
    private:
        Spark(const Spark&);
        Spark& operator=(const Spark&);

        class Notification : public RPC::IRemoteProcess::INotification,
                             public PluginHost::IStateControl::INotification,
                             public Exchange::IBrowser::INotification {

        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(Spark* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
                INTERFACE_ENTRY(Exchange::IBrowser::INotification)
                INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
            END_INTERFACE_MAP

        private:
            virtual void StateChange(const PluginHost::IStateControl::state state) override
            {
                _parent.StateChange(state);
            }
            // Signal changes on the subscribed namespace..
            virtual void LoadFinished(const string& URL) override
            {
            }
            virtual void URLChanged(const string& URL) override
            {
            }
            virtual void Hidden(const bool hidden) override
            {
            }
            virtual void Closure() override
            {
            }
            virtual void Activated(RPC::IRemoteProcess*) override
            {
            }
            virtual void Deactivated(RPC::IRemoteProcess* process) override
            {
                _parent.Deactivated(process);
            }

        private:
            Spark& _parent;
        };

    public:
        class Data : public Core::JSON::Container {
        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , URL()
                , Suspended(false)
            {
                Add(_T("url"), &URL);
                Add(_T("suspended"), &Suspended);
            }
            ~Data()
            {
            }

        public:
            Core::JSON::String URL;
            Core::JSON::Boolean Suspended;
        };

    public:
        Spark()
            : _spark(nullptr)
            , _memory(nullptr)
            , _service(nullptr)
            , _notification(this)
        {
        }
        virtual ~Spark()
        {
        }

    public:
        BEGIN_INTERFACE_MAP(Spark)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(PluginHost::IStateControl, _spark)
        INTERFACE_AGGREGATE(Exchange::IBrowser, _spark)
        INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service);
        virtual void Deinitialize(PluginHost::IShell* service);
        virtual string Information() const;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

    private:
        void StateChange(const PluginHost::IStateControl::state state);
        void Deactivated(RPC::IRemoteProcess* process);

    private:
        uint8_t _skipURL;
        uint32_t _pid;
        Exchange::IBrowser* _spark;
        Exchange::IMemory* _memory;
        PluginHost::IShell* _service;
        Core::Sink<Notification> _notification;
    };
}
} // namespace
