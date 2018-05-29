#include "NetworkControl.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Plugin::NetworkControl::mode)

    { Plugin::NetworkControl::MANUAL,    _TXT("Manual")  },
    { Plugin::NetworkControl::STATIC,    _TXT("Static")  },
    { Plugin::NetworkControl::DYNAMIC,   _TXT("Dynamic") },

ENUM_CONVERSION_END(Plugin::NetworkControl::mode)

namespace Plugin {

    SERVICE_REGISTRATION(NetworkControl, 1, 0);

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<NetworkControl::Entry> > jsonGetNetworkFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType< Core::JSON::ArrayType< NetworkControl::Entry> > > jsonGetNetworksFactory(1);
    static TCHAR NAMESERVER[] = "nameserver ";
	
    static bool AddDNSEntry(std::list< std::pair< uint16_t, Core::NodeId > >& container, const Core::NodeId& element) {
        bool result = false;

        std::list< std::pair< uint16_t, Core::NodeId > >::iterator finder (container.begin());

        while ((finder != container.end()) && (finder->second != element)) { finder++; }

        if (finder == container.end()) {
            result = true;
            container.push_back( std::pair< uint16_t, Core::NodeId > (1, element) );
        }
        else {
            finder->first += 1;
        }

        return (result);
    }

    static bool RemoveDNSEntry(std::list< std::pair< uint16_t, Core::NodeId > >& container, const Core::NodeId& element) {
        bool result = true;

        std::list< std::pair< uint16_t, Core::NodeId > >::iterator finder (container.begin());

        while ((finder != container.end()) && (finder->second != element)) { finder++; }

        if (finder == container.end()) {
            TRACE_L1("How could this be, it does not exist. Time for a refresh. [%s]", element.HostAddress().c_str());
        }
        else if (finder->first == 1) {
            container.erase(finder);
        }
        else {
            result = false;
            finder->first -= 1;
        }

        return (result);
    }
	
    #ifdef __WIN32__
    #pragma warning( disable : 4355 )
    #endif
    NetworkControl::NetworkControl()
        : _skipURL(0)
	, _service(nullptr)
	, _responseTime(0)
	, _retries(0)
        , _dns()
	, _interfaces()
	, _dhcpInterfaces()
	, _observer(Core::ProxyType<AdapterObserver>::Create(this))
    {
    }
    #ifdef __WIN32__
    #pragma warning( default : 4355 )
    #endif

    /* virtual */ NetworkControl::~NetworkControl()
    {
    }

    /* virtual */ const string NetworkControl::Initialize(PluginHost::IShell* service)
    {
        string result;
        Config config;
        config.FromString(service->ConfigLine());

	_service = service;
	_skipURL = static_cast<uint8_t>(service->WebPrefix().length());
	_responseTime = config.TimeOut.Value();
	_dnsFile = config.DNSFile.Value();

	// We will only "open" the DNS resolve file, so of ot does not exist yet, create an empty file.
	Core::File dnsFile(_dnsFile, true);
        if (dnsFile.Exists() == false) {
            if (dnsFile.Create() == false) {
                SYSLOG(PluginHost::Startup, (_T("Could not create DNS configuration file [%s]"), _dnsFile.c_str()));
            }
            else {
                dnsFile.Close();
            }
        }

        Core::JSON::ArrayType<Entry>::Iterator index (config.Interfaces.Elements());

        while (index.Next() == true) {
            if (index.Current().Interface.IsSet() == true) {
		string interfaceName(index.Current().Interface.Value());
                Core::AdapterIterator adapter;
                uint8_t retries = (_responseTime * 2);

		// Some interfaces take some time, to be available. Wait a certain amount 
                // of time in which the interface should come up.
		do {
                	adapter = Core::AdapterIterator (interfaceName);

			if (adapter.IsValid() == false) {
		    		Core::AdapterIterator::Flush();
		    		SleepMs(500);
			}

                } while ( (retries-- != 0) && (adapter.IsValid() == false) );
 	
		if (adapter.IsValid() == false) {
                    SYSLOG(PluginHost::Startup, (_T("Interface [%s], not available"), interfaceName.c_str()));
                }
                else {

                    adapter.Up(true);

                    _dhcpInterfaces.emplace(std::piecewise_construct, 
                        std::make_tuple(interfaceName), 
                        std::make_tuple(Core::ProxyType<DHCPEngine>::Create(this, interfaceName)));
                    _interfaces.emplace(std::piecewise_construct, 
                        std::make_tuple(interfaceName), 
                        std::make_tuple(index.Current()));


                    mode how (index.Current().Mode);
                    if (how == MANUAL) {
                        SYSLOG(PluginHost::Startup, (_T("Interface [%s] activated, no IP associated"), interfaceName.c_str()));
		    }
		    else {
			if (how == DYNAMIC) {
                            SYSLOG(PluginHost::Startup, (_T("Interface [%s] activated, DHCP request issued"), interfaceName.c_str()));
                            Reload(interfaceName, true);
			}
			else {
                            SYSLOG(PluginHost::Startup, (_T("Interface [%s] activated, static IP assigned"), interfaceName.c_str()));
                            Reload(interfaceName, false);
			}
                    }
                }
            }
        }

        if (config.Open.Value() == true) {
            // Find all adapters that are not listed.
            Core::AdapterIterator notListed;

            while (notListed.Next() == true) {
                const string interfaceName (notListed.Name());

                if (_interfaces.find(interfaceName) == _interfaces.end()) {
                    _dhcpInterfaces.emplace(std::piecewise_construct, 
                        std::make_tuple(interfaceName), 
                        std::make_tuple(Core::ProxyType<DHCPEngine>::Create(this, interfaceName)));
                    _interfaces.emplace(std::piecewise_construct, 
                        std::make_tuple(interfaceName), 
                        std::make_tuple(StaticInfo()));
                }
            }
        }

        Core::JSON::ArrayType<Core::JSON::String>::Iterator entries (config.DNS.Elements());

        while (entries.Next() == true) {
            Core::NodeId entry (entries.Current().Value().c_str());

            if (entry.IsValid() == true) {
                _dns.push_back( std::pair<uint16_t, Core::NodeId> (1, entry) );
            }
        }

	// Update the DNS information, before we set the new IP, Do not know who triggers
        // the re-read of this file....
	RefreshDNS();

        // From now on we observer the states of the give interfaces.
        _observer->Open();

        // On success return empty, to indicate there is no error text.
        return (result);
    }

    /* virtual */ void NetworkControl::Deinitialize(PluginHost::IShell* service)
    {

	// Stop observing.
        _observer->Close();

        std::map<const string, Core::ProxyType<DHCPEngine> >::iterator index( _dhcpInterfaces.begin());

        while (index != _dhcpInterfaces.end()) {
            index->second->CleanUp();
            index++;
        }

        _dns.clear();
        _dhcpInterfaces.clear();
        _interfaces.clear();
	_service = nullptr;
    }

    /* virtual */ string NetworkControl::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void NetworkControl::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response>
    NetworkControl::Process(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index( Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [NetworkControl] service.");

        // By default, we skip the first slash
        index.Next();

        if (request.Verb == Web::Request::HTTP_GET) {

            _adminLock.Lock();

            if (index.Next() == true) {


                std::map<const string, StaticInfo>::iterator entry(_interfaces.find(index.Current().Text()));

				if (entry != _interfaces.end()) {

					Core::ProxyType<Web::JSONBodyType<NetworkControl::Entry> > data(jsonGetNetworkFactory.Element());
					
					entry->second.Store(*data);
					data->Interface = entry->first;

					result->Body(data);
				}
				else {
					result->ErrorCode = Web::STATUS_NOT_FOUND;
					result->Message = string(_T("Could not find interface: ")) + index.Current().Text();
				}
            }
            else {
                std::map<const string, StaticInfo>::iterator entry(_interfaces.begin());

				Core::ProxyType<Web::JSONBodyType< Core::JSON::ArrayType< NetworkControl::Entry> > > data(jsonGetNetworksFactory.Element());

                while (entry != _interfaces.end()) {

					Entry& newSlot = data->Add();

					entry->second.Store(newSlot);
					newSlot.Interface = entry->first;
					entry++;
                }
				result->Body(data);
			}

            _adminLock.Unlock();
        }
        else if ( (index.Next() == true) && (request.Verb == Web::Request::HTTP_PUT) ) {
             
            _adminLock.Lock();

            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(index.Current().Text()));

            if ( (entry != _interfaces.end()) && (index.Next() == true) ) {

                bool reload = (index.Current() == _T("Reload"));

                if (((reload == true) && (entry->second.Mode() != STATIC)) || (index.Current() == _T("Request"))) {

                    if (Reload(entry->first, true) == Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = "OK";
                    }
                    else {
                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        result->Message = "Could not activate the server";
                    }
                }
                else if (((reload == true) && (entry->second.Mode() == STATIC)) || (index.Current() == _T("Assign"))) {

                    if (Reload(entry->first, false) == Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = "OK";
                    }
                    else {
                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        result->Message = "Could not activate the server";
                    }
                }
                else if (index.Current() == _T("Up")) {
                    string interfaceName (entry->first);
                    Core::AdapterIterator adapter(interfaceName);

                    if (adapter.IsValid() == false) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = string(_T("Interface: ")) + interfaceName + _T(" not found.");
                    }
                    else {
                        adapter.Up(true);
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = string(_T("OK, ")) + interfaceName + _T(" set UP.");
                    }
                }
                else if (index.Current() == _T("Down")) {
                    string interfaceName (entry->first);
                    Core::AdapterIterator adapter(interfaceName);

                    if (adapter.IsValid() == false) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = string(_T("Interface: ")) + interfaceName + _T(" not found.");
                    }
                    else {
                        adapter.Up(false);
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = string(_T("OK, ")) + interfaceName + _T(" set DOWN.");
                    }
                }
                else if (index.Current() == _T("Flush")) {
                    string interfaceName (entry->first);
                    Core::AdapterIterator adapter(interfaceName);

                    if (adapter.IsValid() == false) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = string(_T("Interface: ")) + interfaceName + _T(" not found.");
                    }
                    else {
			Core::IPV4AddressIterator ipv4Flush (adapter.IPV4Addresses());
			Core::IPV6AddressIterator ipv6Flush (adapter.IPV6Addresses());

			while (ipv4Flush.Next() == true) {
				adapter.Delete(ipv4Flush.Address());
			}
			while (ipv6Flush.Next() == true) {
				adapter.Delete(ipv6Flush.Address());
			}

                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = string(_T("OK, ")) + interfaceName + _T(" set DOWN.");
                    }
                }
 
            }

            _adminLock.Unlock();
        }

        return result;
    }

    uint32_t NetworkControl::SetIP(Core::AdapterIterator& adapter, const Core::IPNode& ipAddress, const Core::NodeId& gateway, const Core::NodeId& broadcast) {

        if (adapter.IsValid() == true) {
            bool addIt = false;

            if (ipAddress.Type() == Core::NodeId::TYPE_IPV4) {
                 Core::IPV4AddressIterator checker (adapter.IPV4Addresses());

                 while ((checker.Next() == true) && (checker.Address() != ipAddress)) /* INTENTINALLY LEFT EMPTY */;

                 addIt = (checker.IsValid() == false);
            }
            if (ipAddress.Type() == Core::NodeId::TYPE_IPV6) {
                 Core::IPV6AddressIterator checker (adapter.IPV6Addresses());

                 while ((checker.Next() == true) && (checker.Address() != ipAddress)) /* INTENTINALLY LEFT EMPTY */;

                 addIt = (checker.IsValid() == false);
            }

            PluginHost::ISubSystem* subSystem = _service->SubSystems();

            ASSERT(subSystem != nullptr);

            if (subSystem != nullptr) {
                if ((subSystem->IsActive(PluginHost::ISubSystem::NETWORK) == false) && (ipAddress.IsLocalInterface() == false)) {
                    subSystem->Set(PluginHost::ISubSystem::NETWORK, nullptr);
                }

                subSystem->Release();
            }
           
            if (addIt == true) {

                TRACE_L1 ("Setting IP: %s", ipAddress.HostAddress().c_str()); 
                adapter.Add(ipAddress);
                adapter.Broadcast(broadcast);
                if (gateway.IsValid() == true) {
                    adapter.Gateway(Core::IPNode(Core::NodeId("0.0.0.0"), 0), gateway);
                }

		Core::AdapterIterator::Flush();

       		string message(string("{ \"interface\": \"") + adapter.Name() + string("\", \"status\":0, \"ip\":\"" + ipAddress.HostAddress() + "\" }"));
                TRACE(Trace::Information, (_T("DHCP Request set on: %s"), adapter.Name().c_str()));

                _service->Notify(message);
            }
            else {
                TRACE_L1 ("No need to set IP: %s", ipAddress.HostAddress().c_str()); 
            }
        }

        return (Core::ERROR_NONE);
    }

    uint32_t NetworkControl::Reload (const string& interfaceName, const bool dynamic) {

        uint32_t result = Core::ERROR_UNKNOWN_KEY;
       
        std::map<const string, StaticInfo>::iterator index(_interfaces.find(interfaceName));

        if (index != _interfaces.end()) {

            if (dynamic == false) { 

                Core::AdapterIterator adapter (interfaceName);

                result = SetIP(adapter, index->second.Address(), index->second.Gateway(), index->second.Broadcast());
            }
            else {
                std::map<const string, Core::ProxyType<DHCPEngine> >::iterator entry(_dhcpInterfaces.find(interfaceName));

                if (entry != _dhcpInterfaces.end()) {
                
                    result = entry->second->Discover();
                }
            }
        }

        return (result); 
 
    }
 
    /* virtual */ uint32_t NetworkControl::AddAddress(const string& interfaceName) {
        return (Reload(interfaceName, true));
    }

   /* virtual */ uint32_t NetworkControl::AddAddress(const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast, const uint8_t netmask) {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        
        std::map<const string, StaticInfo>::iterator index(_interfaces.find(interfaceName));

        if (index != _interfaces.end()) {

            Core::IPNode address(Core::NodeId(IPAddress.c_str()), netmask);
            Core::NodeId ipGateway;
            Core::NodeId ipBroadcast;
            Core::AdapterIterator adapter (interfaceName);

            result = SetIP(adapter, address, ipGateway, ipBroadcast);
        }

        return (result); 
    }

    /* virtual */ uint32_t NetworkControl::RemoveAddress(const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast) {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        
        std::map<const string, StaticInfo>::iterator index(_interfaces.find(interfaceName));

        if (index != _interfaces.end()) {

            Core::NodeId basic(IPAddress.c_str());
            Core::IPNode address(basic, basic.DefaultMask());
            Core::NodeId ipGateway;
            Core::NodeId ipBroadcast;
            Core::AdapterIterator adapter (interfaceName);

            // Todo allow for IP adresses to be removed.
            //result = SetIP(adapter, index->second.Address(), ipGateway, ipBroadcast);
        }

        return (result); 
    }

    /* virtual */ uint32_t NetworkControl::AddDNS(IIPNetwork::IDNSServers* dnsEntries) {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        bool syncDNS = false;

        dnsEntries->Reset();

        _adminLock.Lock();

        while (dnsEntries->Next() == true) {
           
            const string server (dnsEntries->Server()); 

            if (server.empty() == false) {
	        Core::NodeId entry (server.c_str());

                if (entry.IsValid() == true) {
                    syncDNS = AddDNSEntry(_dns, entry) | syncDNS;
                }
            }
        }

        if (syncDNS == true) {
            RefreshDNS();
        }

        _adminLock.Unlock();

        return(result);
    }

    /* virtual */ uint32_t NetworkControl::RemoveDNS(IIPNetwork::IDNSServers* dnsEntries) {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        bool syncDNS = false;

        dnsEntries->Reset();

        _adminLock.Lock();

        while (dnsEntries->Next() == true) {
           
            const string server (dnsEntries->Server()); 

            if (server.empty() == false) {
	        Core::NodeId entry (server.c_str());

                if (entry.IsValid() == true) {
                    syncDNS = RemoveDNSEntry(_dns, entry) | syncDNS;
                }
            }
        }

        if (syncDNS == true) {
            RefreshDNS();
        }

        _adminLock.Unlock();

        return(result);
    }
 
	void NetworkControl::Update(const string& interfaceName) {

		std::map<const string, Core::ProxyType<DHCPEngine> >::const_iterator entry(_dhcpInterfaces.find(interfaceName));

		if (entry != _dhcpInterfaces.end()) {

			if (entry->second->Classification() == DHCPClientImplementation::classifications::CLASSIFICATION_DISCOVER) {

				DHCPClientImplementation::Iterator index(entry->second->Offers());
	
				if (index.Next() == true) {

					std::map<const string, StaticInfo >::iterator info(_interfaces.find(interfaceName));

					const DHCPClientImplementation::Offer& current = (index.Current());

					TRACE_L1("DHCP Source:    %s", current.Source().HostAddress().c_str());
					TRACE_L1("     Address:   %s", current.Address().HostAddress().c_str());
					TRACE_L1("     Broadcast: %s", current.Broadcast().HostAddress().c_str());
					TRACE_L1("     Gateway:   %s", current.Gateway().HostAddress().c_str());
					TRACE_L1("     DNS:       %d", current.DNS().Count());
					TRACE_L1("     Netmask:   %d", current.Netmask());
	
					Core::AdapterIterator adapter(interfaceName);

					ASSERT (info != _interfaces.end());

					if (info != _interfaces.end()) {

						bool update = false;
	
						_adminLock.Lock();

						// First add all new entries.
		        	                DHCPClientImplementation::Offer::DnsIterator servers (current.DNS());
						while (servers.Next() == true) { update = AddDNSEntry(_dns, servers.Current()) | update; }

						// Than remove all old ones.
		                	        servers = info->second.Offer().DNS();
						while (servers.Next() == true) { update = RemoveDNSEntry(_dns, servers.Current()) | update; }

						entry->second->Acknowledge(current.Address());

						_adminLock.Unlock();

						// Add the chosen selection to the interface.
						info->second.Offer(current);

						if (update == true) {
							RefreshDNS();
						}

						SetIP(adapter, Core::IPNode(current.Address(), current.Netmask()), current.Gateway(), current.Broadcast());
                                	}
				}
			}
			else if ( (entry->second->Classification() == DHCPClientImplementation::classifications::CLASSIFICATION_ACK) ||
			          (entry->second->Classification() == DHCPClientImplementation::classifications::CLASSIFICATION_NAK) ) {
				entry->second->Completed();
			}
		}
	}

	void NetworkControl::Expired(const string& interfaceName) {
		std::map<const string, Core::ProxyType<DHCPEngine> >::iterator index(_dhcpInterfaces.find(interfaceName));

		if (index != _dhcpInterfaces.end()) {
			
                    string message(string("{ \"interface\": \"") + index->first + string("\", \"status\":11 }"));
                    TRACE(Trace::Information, (_T("DHCP Request timed out on: %s"), index->first.c_str()));

                    index->second->Completed();
                    _service->Notify(message);
		}
	}

         void NetworkControl::RefreshDNS () {
            Core::DataElementFile file(_dnsFile, Core::DataElementFile::SHAREABLE|Core::DataElementFile::READABLE|Core::DataElementFile::WRITABLE);

            if (file.IsValid() == false) {
                SYSLOG(PluginHost::Startup, (_T("DNS functionality could NOT be updated [%s]"), _dnsFile.c_str()));
            }
            else {
                string data ((_T("#++SECTION: ")) + _service->Callsign() + '\n');
                string endMarker ((_T("#--SECTION: ")) + _service->Callsign() + '\n');
                uint16_t start = 0;
                uint16_t end = 0;
                uint16_t offset = 1;
                uint8_t  index = 0;

                do {
                    index += offset - 1;
                    offset = 0;
 
                    // Find the first comparible character && check the remainder in the next step...
                    while ((index < file.Size()) && (file[index] != data[offset])) { index++; }
                    while ((index < file.Size()) && (offset < data.length()) && (file[index] == data[offset])) { index++; offset++; } 

                } while ( (index < file.Size()) && (offset != data.length()) );

                start  = index - offset;
                offset = 1;

                do {
                    index += offset - 1;
                    offset = 0;

                    // Find the first comparible character && check the remainder in the next step...
                    while ((index < file.Size()) && (file[index] != endMarker[offset])) { index++; }
                    while ((index < file.Size()) && (offset < endMarker.length()) && (file[index] == endMarker[offset])) { index++; offset++; } 

                } while ( (index < file.Size()) && (offset != endMarker.length()) );

                end = index;
                uint16_t reduction = end - start;

                if (reduction != 0) {
                    // move everything after de marker, over the marker sections.
                    ::memcpy (&(file[start]), &file[end], static_cast<uint16_t>(file.Size()) - end);
                }

                offset = (static_cast<uint16_t>(file.Size()) - reduction);

                _adminLock.Lock();

                std::list< std::pair< uint16_t, Core::NodeId > >::const_iterator pointer (_dns.begin());

                while (pointer != _dns.end()) {
                    data += string(NAMESERVER, sizeof(NAMESERVER) - 1) + pointer->second.HostAddress() + '\n';
                    pointer++;
                }

                _adminLock.Unlock();

                data += endMarker;
                file.Size(offset + data.length());
                ::memcpy(&(file[offset]), data.c_str(), data.length());
                file.Sync();

                SYSLOG(PluginHost::Startup, (_T("DNS functionality updated [%s]"), _dnsFile.c_str()));
            }
        }

        void NetworkControl::Activity (const string& interfaceName) {
            string message;
            Core::AdapterIterator adapter(interfaceName);

  	    if (adapter.IsValid() == true) {
		// Send a message with the state of the adapter.
                message = 
                    string(_T("{ \"interface\": \"")) + interfaceName + 
                    string(_T("\", \"running\": \"")) + string(adapter.IsRunning() ? _T("true") : _T("false")) + 
                    string(_T("\", \"up\": \"")) + string(adapter.IsUp() ? _T("true") : _T("false")) + _T("\", \"event\": \"");
                TRACE(Trace::Information, (_T("Adapter change report on: %s"), interfaceName.c_str()));

                _adminLock.Lock();

                if (_interfaces.find(interfaceName) == _interfaces.end()) {
                    _dhcpInterfaces.emplace(std::piecewise_construct, 
                        std::make_tuple(interfaceName), 
                        std::make_tuple(Core::ProxyType<DHCPEngine>::Create(this, interfaceName)));
                    _interfaces.emplace(std::piecewise_construct, 
                        std::make_tuple(interfaceName), 
                        std::make_tuple(StaticInfo()));

                    message += _T("Create\" }");

                    TRACE(Trace::Information, (_T("Added interface: %s"), interfaceName.c_str()));
                }
                else {
                    message += _T("Update\" }");
                    TRACE(Trace::Information, (_T("Updated interface: %s"), interfaceName.c_str()));
                }

                if ((adapter.IsRunning() == true) && (adapter.IsUp() == true)) {
                    std::map<const string, StaticInfo>::iterator index(_interfaces.find(interfaceName));

                    if (index != _interfaces.end()) {

                        mode how (index->second.Mode());
                        if (how != MANUAL) {
                            Reload(interfaceName, how == DYNAMIC);
                        }
                    }
                } 

                _adminLock.Unlock();

	    }
            else {
                // Seems like the adapter disappeared. Remove it.
                _adminLock.Lock();

                std::map<const string, StaticInfo>::iterator info (_interfaces.find(interfaceName));
                if (info != _interfaces.end()) {
                    _interfaces.erase (info);
                }

	        std::map<const string, Core::ProxyType<DHCPEngine> >::iterator dhcp (_dhcpInterfaces.find(interfaceName));
                if (dhcp != _dhcpInterfaces.end()) {
                    _dhcpInterfaces.erase (dhcp);
                }

                _adminLock.Unlock();

                TRACE(Trace::Information, (_T("Removed interface: %s"), interfaceName.c_str()));

                message = 
                    string(_T("{ \"interface\": \"")) + interfaceName + string(_T("\", \"running\": \"false\", \"up\": \"false\", \"event\": \"Delete\" }"));
            }

            _service->Notify(message);
	}

} // namespace Plugin
} // namespace WPEFramework
