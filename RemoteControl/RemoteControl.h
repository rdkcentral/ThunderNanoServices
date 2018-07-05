#ifndef __PLUGINWEBPROXY_H
#define __PLUGINWEBPROXY_H

#include "Module.h"
#include "RemoteAdministrator.h"
#include <interfaces/IKeyHandler.h>

namespace WPEFramework {
namespace Plugin {

    class RemoteControl : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::IKeyHandler {
    private:
        RemoteControl(const RemoteControl&);
        RemoteControl& operator=(const RemoteControl&);

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            class Device : public Core::JSON::Container {
            private:
                Device& operator=(const Device&) = delete;

            public:
                Device()
                    : Core::JSON::Container()
                    , Name()
                    , MapFile()
                    , PassOn(false)
                    , Settings()
                {
                    Add(_T("name"), &Name);
                    Add(_T("mapfile"), &MapFile);
                    Add(_T("passon"), &PassOn);
                    Add(_T("settings"), &Settings);
                }
                Device(const Device& copy)
                    : Core::JSON::Container()
                    , Name(copy.Name)
                    , MapFile(copy.MapFile)
                    , PassOn(copy.PassOn)
                    , Settings(copy.Settings)
                {
                    Add(_T("name"), &Name);
                    Add(_T("mapfile"), &MapFile);
                    Add(_T("passon"), &PassOn);
                    Add(_T("settings"), &Settings);
                }
                ~Device()
                {
                }

                Core::JSON::String Name;
                Core::JSON::String MapFile;
                Core::JSON::Boolean PassOn;
                Core::JSON::String Settings;
            };

        public:
            Config()
                : Core::JSON::Container()
                , MapFile("keymap.json")
                , PassOn(false)
                , RepeatStart(500)
                , RepeatInterval(100)
                , Devices()
            {
                Add(_T("mapfile"), &MapFile);
                Add(_T("passon"), &PassOn);
                Add(_T("repeatstart"), &RepeatStart);
                Add(_T("repeatinterval"), &RepeatInterval);
                Add(_T("devices"), &Devices);
                Add(_T("virtuals"), &Virtuals);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String MapFile;
            Core::JSON::Boolean PassOn;
            Core::JSON::DecUInt16 RepeatStart;
            Core::JSON::DecUInt16 RepeatInterval;
            Core::JSON::ArrayType<Device> Devices;
            Core::JSON::ArrayType<Device> Virtuals;
        };

        class Data : public Core::JSON::Container {

        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , Devices()
            {
                Add(_T("devices"), &Devices);
            }

            virtual ~Data()
            {
            }

        public:
            Core::JSON::ArrayType<Core::JSON::String> Devices;
        };

    public:
        RemoteControl();
        virtual ~RemoteControl();

        BEGIN_INTERFACE_MAP(RemoteControl)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(Exchange::IKeyHandler)
        END_INTERFACE_MAP

    public:
        bool IsVirtualDevice(const string& name) const
        {
            return (std::find(_virtualDevices.begin(), _virtualDevices.end(), name) != _virtualDevices.end());
        }
        bool IsPhysicalDevice(const string& name) const
        {
            Remotes::RemoteAdministrator::Iterator index(Remotes::RemoteAdministrator::Instance().Producers());

            while ((index.Next() == true) && (name != index.Current()->Name())) /* Intentionally empty */;

            return (index.IsValid());
        }

        //	IPlugin methods
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

        //      IKeyHandler methods
        // -------------------------------------------------------------------------------------------------------
        // Whnever a key is pressed or release, let this plugin now, it will take the proper arrangements and timings
        // to announce this key event to the linux system. Repeat event is triggered by the watchdog implementation
        // in this plugin. No need to signal this.
        virtual uint32_t KeyEvent(const bool pressed, const uint32_t code, const string& table);

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index, const Web::Request& request) const;
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        const string FindDevice(Core::TextSegmentIterator& index) const;
        bool ParseRequestBody(const Web::Request& request, uint32_t& code, uint16_t& key, uint32_t& modifiers);
        Core::ProxyType<Web::IBody> CreateResponseBody(uint32_t code, uint32_t key, uint16_t modifiers) const;

    private:
        uint32_t _skipURL;
        std::list<string> _virtualDevices;
        PluginHost::VirtualInput* _inputHandler;
        string _persistentPath;
    };
}
}

#endif // __PLUGINWEBPROXY_H
