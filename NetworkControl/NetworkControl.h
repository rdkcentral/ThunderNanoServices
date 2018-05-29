#ifndef PLUGIN_NETWORKCONTROL_H
#define PLUGIN_NETWORKCONTROL_H

#include "Module.h"
#include "DHCPClientImplementation.h"

#include <interfaces/IIPNetwork.h>

namespace WPEFramework {
namespace Plugin {

    class NetworkControl : 
        public Exchange::IIPNetwork, 
        public PluginHost::IPlugin, 
        public PluginHost::IWeb {
    public:
	enum mode {
		MANUAL,
		STATIC,
		DYNAMIC
	};

	class Entry : public Core::JSON::Container {
	private:
		Entry & operator=(const Entry&) = delete;

	public:
		Entry()
			: Core::JSON::Container()
			, Interface()
			, Mode(MANUAL)
			, Address()
			, Mask(32)
			, Gateway()
			, _broadcast() {
			Add(_T("interface"), &Interface);
			Add(_T("mode"), &Mode);
			Add(_T("address"), &Address);
			Add(_T("mask"), &Mask);
			Add(_T("gateway"), &Gateway);
			Add(_T("broadcast"), &_broadcast);
		}
		Entry(const Entry& copy)
			: Core::JSON::Container()
			, Interface(copy.Interface)
			, Mode(copy.Mode)
			, Address(copy.Address)
			, Mask(copy.Mask)
			, Gateway(copy.Gateway)
			, _broadcast(copy._broadcast) {
			Add(_T("interface"), &Interface);
			Add(_T("mode"), &Mode);
			Add(_T("address"), &Address);
			Add(_T("mask"), &Mask);
			Add(_T("gateway"), &Gateway);
			Add(_T("broadcast"), &_broadcast);
		}
		virtual ~Entry() {
		}

	public:
		Core::JSON::String Interface;
		Core::JSON::EnumType<mode> Mode;
		Core::JSON::String Address;
		Core::JSON::DecUInt8 Mask;
		Core::JSON::String Gateway;

	public:
		void Broadcast(const Core::NodeId& address) {
			_broadcast = address.HostAddress();
		}
		Core::NodeId Broadcast() const {
			Core::NodeId result;

			if (_broadcast.IsSet() == false) {
							
				Core::IPNode address (Core::NodeId(Address.Value().c_str()), Mask.Value());

				result = address.Broadcast();
						
			}
			else {
				result = Core::NodeId(_broadcast.Value().c_str());
			}

			return (result);
		}

	private:
		Core::JSON::String _broadcast;
	};

    private:
        class AdapterObserver : 
            public Core::IDispatch,
            public WPEFramework::Core::AdapterObserver::INotification {
        private:
            AdapterObserver() = delete;
            AdapterObserver(const AdapterObserver&) = delete;
            AdapterObserver& operator= (const AdapterObserver&) = delete;

        public:
            AdapterObserver(NetworkControl* parent)
                : _parent(*parent)
                , _adminLock()
	        , _observer(this)
                , _reporting() {
                ASSERT (parent != nullptr);
            }
            virtual ~AdapterObserver() {
            }

        public:
            void Open() {
                _observer.Open();
            }
            void Close() {
	        Core::ProxyType<Core::IDispatch> job(*this);
                _observer.Close();
                
                _adminLock.Lock();

	        PluginHost::WorkerPool::Instance().Revoke(job);

		_reporting.clear();

                _adminLock.Unlock();
            }
            virtual void Event(const string& interface) override {

                _adminLock.Lock();

                if (std::find(_reporting.begin(), _reporting.end(), interface) == _reporting.end()) {
                    // We need to add this interface, it is currently not present.
                    _reporting.push_back(interface);

                    // If this is the first entry, we need to submit a job for processing
                    if (_reporting.size() == 1) {
			// These events tend to "dender" a lot. Give it some time
			Core::Time entry(Core::Time::Now().Add(100));
			Core::ProxyType<Core::IDispatch> job(*this);

		        PluginHost::WorkerPool::Instance().Schedule(entry, job);
                    }
                }

                _adminLock.Unlock();
            }
	    virtual void Dispatch() override {
		// Yippie a yee, we have an interface notification:
                _adminLock.Lock();
                while (_reporting.size() != 0) {
                    const string interfaceName (_reporting.front());
                    _reporting.pop_front();
                    _adminLock.Unlock();

                    _parent.Activity(interfaceName);

                    _adminLock.Lock();
                }
                _adminLock.Unlock();
            }

	private:
            NetworkControl& _parent;
            Core::CriticalSection _adminLock;
	    Core::AdapterObserver _observer;
            std::list<string> _reporting;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
		, DNSFile("/etc/resolv.conf")
                , Interfaces ()
		, DNS()
		, TimeOut(5)
                , Open(true) {
                Add(_T("dnsfile"), &DNSFile);
                Add(_T("interfaces"), &Interfaces);
		Add(_T("timeout"), &TimeOut);
		Add(_T("open"), &Open);
		Add(_T("dns"), &DNS);
	    }
            ~Config() {
            }

        public:
            Core::JSON::String DNSFile;
            Core::JSON::ArrayType< Entry > Interfaces;
            Core::JSON::ArrayType< Core::JSON::String > DNS;
	    Core::JSON::DecUInt8 TimeOut;
            Core::JSON::Boolean Open;
	};

        class StaticInfo { 
        private:
            StaticInfo& operator= (const StaticInfo&) = delete;

        public:
            StaticInfo () 
                : _mode(MANUAL)
                , _address()
                , _gateway()
                , _broadcast() {
            }
            StaticInfo(const Entry& info) 
                : _mode(info.Mode.Value())
                , _address(Core::IPNode(Core::NodeId(info.Address.Value().c_str()), info.Mask.Value()))
                , _gateway(Core::NodeId(info.Gateway.Value().c_str()))
                , _broadcast(info.Broadcast()) {
            }
            StaticInfo(const StaticInfo& copy) 
                : _mode(copy._mode)
                , _address(copy._address)
                , _gateway(copy._gateway)
                , _broadcast(copy._broadcast) {
            }
            ~StaticInfo() {
            }

        public:
            inline mode Mode() const {
                return (_mode);
            }
            inline const Core::IPNode& Address() const {
                return (_address);
            }
            inline const Core::NodeId& Gateway() const {
                return (_gateway);
            }
            inline const Core::NodeId& Broadcast() const {
                return (_broadcast);
            }
            inline const DHCPClientImplementation::Offer& Offer() const {
                return (_offer);
            }
            inline void Offer(const DHCPClientImplementation::Offer& offer) {
                _offer = offer;
            }
	    inline void Store(Entry& info) {
		info.Mode = _mode;
		info.Address = _address.HostAddress();
		info.Mask = _address.Mask();
		info.Gateway = _gateway.HostAddress();
		info.Broadcast(_broadcast);
 	    }

        private:
            mode _mode;
            Core::IPNode _address;
            Core::NodeId _gateway;
            Core::NodeId _broadcast;
            DHCPClientImplementation::Offer _offer;
        };

		class DHCPEngine : public Core::IDispatch, public DHCPClientImplementation::ICallback {
		private:
			DHCPEngine() = delete;
			DHCPEngine(const DHCPEngine&) = delete;
			DHCPEngine& operator=(const DHCPEngine&) = delete;

		public:
			DHCPEngine(NetworkControl* parent, const string& interfaceName)
				: _parent(*parent)
				, _retries(0)
				, _client(interfaceName, this) {
			}
			~DHCPEngine() {
			}

		public:
			inline DHCPClientImplementation::classifications Classification() const {
				return (_client.Classification());
			}
			inline uint32_t Discover() {
				return (Discover(Core::NodeId()));
			}
			inline uint32_t Discover(const Core::NodeId& preferred) {

				uint32_t result = _client.Discover(preferred);
				Core::Time entry(Core::Time::Now().Add(_parent.ResponseTime() * 1000));
				Core::ProxyType<Core::IDispatch> job(*this);

				if (result == Core::ERROR_NONE) {

					_retries = _parent.ResponseTime() + 1;

					// Submit a job, as watchdog.
					PluginHost::WorkerPool::Instance().Schedule(entry, job);
				}

				return (result);
			}
			inline void Acknowledge(const Core::NodeId& selected) {
				_client.Request(selected);
				// Don't know if there is still a timer pending, but lets kill it..
				CleanUp();
			}

			inline void CleanUp() {
				PluginHost::WorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(*this));
			}
			inline void Completed() {
				_client.Completed();

			}
			inline DHCPClientImplementation::Iterator Offers() const {
				return (_client.Offers());
			}
			virtual void Dispatch(const string& name) override {
				_parent.Update(name);
			}
			virtual void Dispatch() override {

				if (_retries > 0) {

					Core::Time entry(Core::Time::Now().Add(1000));
					Core::ProxyType<Core::IDispatch> job(*this);

					_retries--;

					_client.Resend();

					// Submit a job, as watchdog.
					PluginHost::WorkerPool::Instance().Schedule(entry, job);
				}
				else {

					_parent.Expired(_client.Interface());
				}
			}

		private:
			NetworkControl& _parent;
			uint8_t _retries;
			DHCPClientImplementation _client;
		};
 
    private:
        NetworkControl(const NetworkControl&) = delete;
        NetworkControl& operator=(const NetworkControl&) = delete;

    public:
        NetworkControl();
        virtual ~NetworkControl();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(NetworkControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
	    INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(Exchange::IIPNetwork) 
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

        //   IIPNetwork methods
        // -------------------------------------------------------------------------------------------------------
        virtual uint32_t AddAddress(const string& interfaceName) override;
        virtual uint32_t AddAddress(const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast, const uint8_t netmask) override;
        virtual uint32_t RemoveAddress(const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast) override;
        virtual uint32_t AddDNS(IIPNetwork::IDNSServers* dnsEntries) override;
        virtual uint32_t RemoveDNS(IIPNetwork::IDNSServers* dnsEntries) override;
 
    private:
        uint32_t Reload (const string& interfaceName, const bool dynamic);
        uint32_t SetIP(Core::AdapterIterator& adapter, const Core::IPNode& ipAddress, const Core::NodeId& gateway, const Core::NodeId& broadcast);
	void Update(const string& interfaceName);
	void Expired(const string& interfaceName);
        void RefreshDNS ();
        void Activity (const string& interface);
        uint16_t DeleteSection (Core::DataElementFile& file, const string& startMarker, const string& endMarker);
	uint8_t ResponseTime() const {
		return(_responseTime);
	}
	uint8_t Retries() const {
		return(_retries);
	}

    private:
        Core::CriticalSection _adminLock;
        uint16_t _skipURL;
	PluginHost::IShell* _service;
	uint8_t _responseTime;
	uint8_t _retries;
        string _dnsFile;
        std::list< std::pair<uint16_t, Core::NodeId> > _dns;
        std::map<const string, StaticInfo> _interfaces;
	std::map<const string, Core::ProxyType<DHCPEngine> > _dhcpInterfaces;
	Core::ProxyType<AdapterObserver> _observer;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // PLUGIN_NETWORKCONTROL_H
