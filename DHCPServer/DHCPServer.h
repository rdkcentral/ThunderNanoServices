#ifndef PLUGIN_DHCPSERVER_H
#define PLUGIN_DHCPSERVER_H

#include "Module.h"
#include "DHCPServerImplementation.h"

namespace WPEFramework {
namespace Plugin {

    class DHCPServer : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        class Data : public Core::JSON::Container {
        private:
            Data(Data const& other) = delete;
            Data& operator=(Data const& other) = delete;

        public:
            class Server : public Core::JSON::Container {
            private:
                Server& operator= (const Server&) = delete;

            public:
                class Lease : public Core::JSON::Container {
                private:
                    Lease& operator= (const Lease&) = delete;

                public:
                    Lease ()
                        : Core::JSON::Container()
                        , Name()
                        , IPAddress()
                        , Expires() {
                        Add(_T("name"), &Name);
                        Add(_T("ip"), &IPAddress);
                        Add(_T("expires"), &Expires);
                    }
                    Lease(const Lease& copy)
                        : Core::JSON::Container()
                        , Name(copy.Name)
                        , IPAddress(copy.IPAddress)
                        , Expires(copy.Expires) {
                        Add(_T("name"), &Name);
                        Add(_T("ip"), &IPAddress);
                        Add(_T("expires"), &Expires);
                    }
                    virtual ~Lease() {
                    }

		public:
                    void Set(const DHCPServerImplementation::Lease& lease) {
                        Name = lease.Id().Text();
                        IPAddress = lease.Address().HostAddress();
                        Expires = lease.Expiration();
                    }
 
                public:
                    Core::JSON::String Name;
                    Core::JSON::String IPAddress;
                    Core::JSON::DecUInt64 Expires;
                };

            public:
                Server ()
                    : Core::JSON::Container()
                    , Interface ()
                    , Begin()
                    , End()
	            , Router()
                    , Active(false)
                    , Leases() {
                    Add(_T("interface"), &Interface);
                    Add(_T("begin"), &Begin);
                    Add(_T("end"), &End);
                    Add(_T("router"), &Router);
                    Add(_T("active"), &Active);
                    Add(_T("leases"), &Leases);
                }
                Server(const Server& copy)
                    : Core::JSON::Container()
                    , Interface(copy.Interface)
                    , Begin(copy.Begin)
                    , End(copy.End)
		    , Router(copy.Router)
                    , Active(copy.Active)
                    , Leases() {
                    Add(_T("interface"), &Interface);
                    Add(_T("begin"), &Begin);
                    Add(_T("end"), &End);
                    Add(_T("router"), &Router);
                    Add(_T("active"), &Active);
                    Add(_T("leases"), &Leases);
                }
                virtual ~Server() {
                }

            public:
                void Set (const DHCPServerImplementation& server) {
                    Interface = server.Interface();
                    Begin = server.BeginPool().HostAddress();
                    End = server.EndPool().HostAddress();
		    Router = server.Router().HostAddress();
                    Active = server.IsActive();

                    DHCPServerImplementation::Iterator index (server.Leases());

                    while (index.Next() == true) {
                        Leases.Add().Set(index.Current());
                    }
                }

            public:
                Core::JSON::String Interface;
                Core::JSON::String Begin;
                Core::JSON::String End;
                Core::JSON::String Router;
                Core::JSON::Boolean Active;
                Core::JSON::ArrayType< Lease > Leases;
            };

        public:
            Data() {
                Add(_T("servers"), &Servers);
            }
            virtual ~Data() {
            }

        public:
           Core::JSON::ArrayType< Server > Servers;
        };

    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            class Server : public Core::JSON::Container {
            private:
                Server& operator=(const Server&) = delete;

            public:
                Server () 
                    : Core::JSON::Container()
                    , Interface()
                    , PoolStart(0)
                    , PoolSize(0)
                    , Router(0)
                    , Active(false) {
                    Add(_T("interface"), &Interface);
                    Add(_T("poolstart"), &PoolStart);
                    Add(_T("poolsize"), &PoolSize);
                    Add(_T("router"), &Router);
                    Add(_T("active"), &Active);
                }
                Server(const Server& copy) 
                    : Core::JSON::Container()
                    , Interface(copy.Interface)
                    , PoolStart(copy.PoolStart)
                    , PoolSize(copy.PoolSize)
                    , Router(copy.Router)
                    , Active(copy.Active) {
                    Add(_T("interface"), &Interface);
                    Add(_T("poolstart"), &PoolStart);
                    Add(_T("poolsize"), &PoolSize);
                    Add(_T("router"), &Router);
                    Add(_T("active"), &Active);
                }
                virtual ~Server() {
                }

            public:
                Core::JSON::String Interface;
                Core::JSON::DecUInt32 PoolStart;
                Core::JSON::DecUInt32 PoolSize;
                Core::JSON::DecUInt32 Router;
                Core::JSON::Boolean Active;
            };

        public:
            Config()
                : Core::JSON::Container()
                , Name()
		, DNS()
                , Servers() {
                Add(_T("name"), &Name);
                Add(_T("dns"), &DNS);
                Add(_T("servers"), &Servers);
            }
            ~Config() {
            }

        public:
            Core::JSON::String Name;
            Core::JSON::String DNS;
            Core::JSON::ArrayType< Server > Servers;
        };

    private:
        DHCPServer(const DHCPServer&) = delete;
        DHCPServer& operator=(const DHCPServer&) = delete;

    public:
        DHCPServer();
        virtual ~DHCPServer();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(DHCPServer)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        uint16_t _skipURL;
	std::map<const string, DHCPServerImplementation> _servers;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // PLUGIN_DHCPSERVER_H
