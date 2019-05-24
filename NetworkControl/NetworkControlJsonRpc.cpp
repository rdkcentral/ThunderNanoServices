
// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#include <interfaces/json/JsonData_NetworkControl.h>
#include "NetworkControl.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::NetworkControl;

    // Registration
    //

    void NetworkControl::RegisterAll()
    {
        Register<NetworkParamsInfo,Core::JSON::ArrayType<NetworkResultData>>(_T("network"), &NetworkControl::endpoint_network, this);
        Register<NetworkParamsInfo,void>(_T("reload"), &NetworkControl::endpoint_reload, this);
        Register<NetworkParamsInfo,void>(_T("request"), &NetworkControl::endpoint_request, this);
        Register<NetworkParamsInfo,void>(_T("assign"), &NetworkControl::endpoint_assign, this);
        Register<NetworkParamsInfo,void>(_T("up"), &NetworkControl::endpoint_up, this);
        Register<NetworkParamsInfo,void>(_T("down"), &NetworkControl::endpoint_down, this);
        Register<NetworkParamsInfo,void>(_T("flush"), &NetworkControl::endpoint_flush, this);
    }

    void NetworkControl::UnregisterAll()
    {
        Unregister(_T("flush"));
        Unregister(_T("down"));
        Unregister(_T("up"));
        Unregister(_T("assign"));
        Unregister(_T("request"));
        Unregister(_T("reload"));
        Unregister(_T("network"));
    }

    // API implementation
    //

    // Retrieves the actual network information for targeted network interface, if network interface is not given, all network interfaces are returned.
    uint32_t NetworkControl::endpoint_network(const NetworkParamsInfo& params, Core::JSON::ArrayType<NetworkResultData>& response)
    {
        uint32_t result = Core::ERROR_NONE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                NetworkResultData data;
                data.Interface = entry->first;
                data.Mode = entry->second.Mode();
                data.Address = entry->second.Address().HostAddress();
                data.Mask = entry->second.Address().Mask();
                data.Gateway = entry->second.Gateway().HostAddress();
                data.Broadcast = entry->second.Broadcast().HostAddress();

                response.Add(data);
            } else {
                result = Core::ERROR_UNAVAILABLE;
            }
        } else {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.begin());

            while (entry != _interfaces.end()) {
                NetworkResultData data;
                data.Interface = entry->first;
                data.Mode = entry->second.Mode();
                data.Address = entry->second.Address().HostAddress();
                data.Mask = entry->second.Address().Mask();
                data.Gateway = entry->second.Gateway().HostAddress();
                data.Broadcast = entry->second.Broadcast().HostAddress();
                response.Add(data);

                entry++;
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Reload static and non-static network interface adapter.
    uint32_t NetworkControl::endpoint_reload(const NetworkParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                if (entry->second.Mode() == NetworkResultData::ModeType::STATIC) {
                    result = Reload(entry->first, false);
                } else {
                    result = Reload(entry->first, true);
                }
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Reload non-static network interface adapter.
    uint32_t NetworkControl::endpoint_request(const NetworkParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                result = Reload(entry->first, true);
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Reload static network interface adapter.
    uint32_t NetworkControl::endpoint_assign(const NetworkParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                result = Reload(entry->first, false);
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Set up network interface adapter.
    uint32_t NetworkControl::endpoint_up(const NetworkParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                Core::AdapterIterator adapter(entry->first);
                if (adapter.IsValid() == true) {
                    adapter.Up(true);
                    result = Core::ERROR_NONE;
                }
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Set down network interface adapter.
    uint32_t NetworkControl::endpoint_down(const NetworkParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                Core::AdapterIterator adapter(entry->first);
                if (adapter.IsValid() == true) {
                    adapter.Up(false);
                    result = Core::ERROR_NONE;
                }
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Flush network interface adapter.
    uint32_t NetworkControl::endpoint_flush(const NetworkParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                Core::AdapterIterator adapter(entry->first);
                if (adapter.IsValid() == true) {
                    Core::IPV4AddressIterator ipv4Flush(adapter.IPV4Addresses());
                    Core::IPV6AddressIterator ipv6Flush(adapter.IPV6Addresses());

                    while (ipv4Flush.Next() == true) {
                        adapter.Delete(ipv4Flush.Address());
                    }
                    while (ipv6Flush.Next() == true) {
                        adapter.Delete(ipv6Flush.Address());
                    }
                    result = Core::ERROR_NONE;
                }
            }
        }

        _adminLock.Unlock();

        return result;
    }

} // namespace Plugin

} // namespace WPEFramework

