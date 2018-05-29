#ifndef __PLUGINWEBSHELL_H
#define __PLUGINWEBSHELL_H

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class SessionMonitor;

    class WebShell : public PluginHost::IPluginExtended, public PluginHost::IChannel {
    private:
        WebShell(const WebShell&) = delete;
        WebShell& operator=(const WebShell&) = delete;

    public:
    class Connectivity {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Connectivity (const Connectivity& a_Copy) = delete;
        Connectivity& operator=(const Connectivity& a_RHS) = delete;

    public:
         Connectivity(const TCHAR formatter[], ...)
         {
             va_list ap;
             va_start(ap, formatter);
             Trace::Format(_text, formatter, ap);
             va_end(ap);
        }
        Connectivity(const string& text) :  _text(Core::ToString(text)) {
        }
        ~Connectivity()
        {
        }

    public:
        inline const char* Data() const {
            return (_text.c_str());
        }
        inline uint16_t Length() const {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

    // Trace class for internal information of WPEFramework
    class Interactivity {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Interactivity (const Interactivity& a_Copy) = delete;
        Interactivity& operator=(const Interactivity& a_RHS) = delete;

    public:
         Interactivity(const TCHAR formatter[], ...)
         {
             va_list ap;
             va_start(ap, formatter);
             Trace::Format(_text, formatter, ap);
             va_end(ap);
        }
        Interactivity(const string& text) :  _text(Core::ToString(text)) {
        }
        ~Interactivity()
        {
        }

    public:
        inline const char* Data() const {
            return (_text.c_str());
        }
        inline uint16_t Length() const {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connections(10)
            {
                Add(_T("connections"), &Connections);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Connections;
        };

    public:
        WebShell()
            : _config()
            , _sessionMonitor(nullptr)
        {
        }
        virtual ~WebShell()
        {
        }

        BEGIN_INTERFACE_MAP(WebShell)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IPluginExtended)
        INTERFACE_ENTRY(PluginHost::IChannel)
        END_INTERFACE_MAP

    public:
        //	IPluginExtended methods
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

        // Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
        // Whenever the channel is closed, it is reported via the detach method.
        virtual bool Attach(PluginHost::Channel& channel);
        virtual void Detach(PluginHost::Channel& channel);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const;

        //	IChannel methods
        // -------------------------------------------------------------------------------------------------------
        // Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of receiving
        // raw data for the plugin. Raw data received on this link will be exposed to the plugin via this interface.
        virtual uint32_t Inbound(const uint32_t ID, const uint8_t data[], const uint16_t length);

        // Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of sending
        // raw data to the initiator of the websocket.
        virtual uint32_t Outbound(const uint32_t ID, uint8_t data[], const uint16_t length) const;

    private:
        void Input  (Core::ProxyType< Core::Process>& process);
        void Output (Core::ProxyType< Core::Process>& process);
        void Error  (Core::ProxyType< Core::Process>& process);

    private:
        string _prefix;
        Config _config;
        SessionMonitor* _sessionMonitor;
    };
}
}

#endif // __PLUGINWESHELL_H
