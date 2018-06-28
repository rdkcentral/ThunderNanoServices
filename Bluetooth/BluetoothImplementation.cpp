#include "BluetoothImplementation.h"

namespace WPEFramework {

namespace Plugin {

    std::map<string, string> BluetoothImplementation::_discoveredDeviceIdMap;
    std::map<string, string> BluetoothImplementation::_pairedDeviceIdMap;
    Core::JSON::ArrayType<BTDeviceList::BTDeviceInfo> BluetoothImplementation::_jsonDiscoveredDevices;
    Core::JSON::ArrayType<BTDeviceList::BTDeviceInfo> BluetoothImplementation::_jsonPairedDevices;

    bool BluetoothImplementation::ConfigureBTAdapter()
    {
        _dbusConnection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL,  NULL);
        if (!_dbusConnection) {
            TRACE(Trace::Error, ("Failed to acquire dbus handle"));
            return false;
        }

        if (!PowerBTAdapter(true)) {
            TRACE(Trace::Error, ("Failed to switch on bluetooth adaptor"));
            return false;
        }

        return true;
    }

    bool BluetoothImplementation::PowerBTAdapter(bool poweringUp)
    {
        GError* dbusError = nullptr;
        GVariant* dbusReply = nullptr;

        dbusReply = g_dbus_connection_call_sync(_dbusConnection, BLUEZ_INTERFACE, BLUEZ_OBJECT_HCI, DBUS_PROP_INTERFACE, "Set", g_variant_new("(ssv)", BLUEZ_INTERFACE_ADAPTER, "Powered", g_variant_new("b", poweringUp)), nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &dbusError);
        if (!dbusReply) {
            TRACE(Trace::Error, ("Failed powerOnBTAdapter dbus call. Error msg :  %s", dbusError->message));
            return false;
        }

        return true;
    }

    bool BluetoothImplementation::SubscribeDBusSignals()
    {
        _interfacesAdded = g_dbus_connection_signal_subscribe(_dbusConnection, BLUEZ_INTERFACE, DBUS_INTERFACE_OBJECT_MANAGER, "InterfacesAdded", ROOT_OBJECT, nullptr, G_DBUS_SIGNAL_FLAGS_NONE, InterfacesAdded, nullptr, nullptr);
        if (!_interfacesAdded) {
            TRACE(Trace::Error, ("Failed to subscribe InterfacesAdded signal"));
            return false;
        }

        _interfacesRemoved = g_dbus_connection_signal_subscribe(_dbusConnection, BLUEZ_INTERFACE, DBUS_INTERFACE_OBJECT_MANAGER, "InterfacesRemoved", ROOT_OBJECT, nullptr, G_DBUS_SIGNAL_FLAGS_NONE, InterfacesRemoved, nullptr, nullptr);
        if (!_interfacesRemoved) {
            TRACE(Trace::Error, ("Failed to subscribe InterfacesRemoved signal"));
            return false;
        }

        return true;
    }

    bool BluetoothImplementation::UnsubscribeDBusSignals()
    {
        g_dbus_connection_signal_unsubscribe(_dbusConnection, _interfacesAdded);
        g_dbus_connection_signal_unsubscribe(_dbusConnection, _interfacesRemoved);

        return true;
    }

    void BluetoothImplementation::InterfacesAdded(GDBusConnection* connection, const gchar* senderName, const gchar* objectPath, const gchar* interfaceName, const gchar* signalName, GVariant* parameters, gpointer userData)
    {
        string deviceId;
        string deviceName;
        string deviceAddress;
        string devicePropertyType;
        string bluezDeviceName;
        GVariantIter* iterator1;
        GVariantIter* iterator2;
        GVariant* devicePropertyValue;
        BTDeviceList::BTDeviceInfo jsonDeviceInfo;

        jsonDeviceInfo.Name = "";
        g_variant_get(parameters, "(oa{sa{sv}})", &deviceId, &iterator1);
        while (g_variant_iter_loop(iterator1, "{sa{sv}}", &bluezDeviceName, &iterator2)) {
            while (g_variant_iter_loop(iterator2, "{sv}", &devicePropertyType, &devicePropertyValue)) {
                if (strcmp(devicePropertyType.c_str(), "Address") == 0) {
                    g_variant_get(devicePropertyValue, "s", &deviceAddress);
                    jsonDeviceInfo.Address = deviceAddress;
                }

                if (strcmp(devicePropertyType.c_str(), "Name") == 0) {
                    g_variant_get(devicePropertyValue, "s", &deviceName);
                    jsonDeviceInfo.Name = deviceName;
                }
            }
        }
        if (_discoveredDeviceIdMap.find(deviceAddress.c_str()) == _discoveredDeviceIdMap.end()) {
             TRACE_L1("Added BT Device. Device ID : [%s]", deviceAddress.c_str());
            _discoveredDeviceIdMap.insert(std::make_pair(deviceAddress.c_str(), deviceId.c_str()));
            _jsonDiscoveredDevices.Add(jsonDeviceInfo);
        }
    }

    void BluetoothImplementation::InterfacesRemoved(GDBusConnection* connection, const gchar* senderName, const gchar* objectPath, const gchar* interfaceName, const gchar* signalName, GVariant* parameters, gpointer userData)
    {
        string deviceId;

        g_variant_get(parameters, "(oas)", &deviceId, nullptr);
        const auto& iterator = _discoveredDeviceIdMap.find(deviceId.c_str());
        if (iterator != _discoveredDeviceIdMap.end()) {
            _discoveredDeviceIdMap.erase(iterator);
            TRACE_L1("Removed BT Device. Device ID : [%s]", deviceId.c_str());
        }
    }

    bool BluetoothImplementation::StartDiscovery()
    {
        GError* dbusError = nullptr;
        GVariant* dbusReply = nullptr;

        dbusReply = g_dbus_connection_call_sync(_dbusConnection, BLUEZ_INTERFACE, BLUEZ_OBJECT_HCI, BLUEZ_INTERFACE_ADAPTER, "StartDiscovery", nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &dbusError);
        if (!dbusReply) {
            TRACE(Trace::Error, ("Failed startDiscovery dbus call. Error msg :  %s", dbusError->message));
            _isScanning = false;
            return false;
        }

        TRACE(Trace::Information, ("Scanning BT Devices..."));
        return true;
    }

    bool BluetoothImplementation::Scan()
    {
        if (!_isScanning) {
            TRACE(Trace::Information, ("Start BT Scan"));
            // Clearing previously discovered devices.
            _discoveredDeviceIdMap.clear();
            _jsonDiscoveredDevices.Clear();
            Run();
            _isScanning = true;
            return true;
        } else {
            TRACE(Trace::Error, ("Scanning already started!"));
            return false;
        }
    }

    bool BluetoothImplementation::StopScan()
    {
        GError* dbusError = nullptr;
        GVariant* dbusReply = nullptr;

        if (_isScanning) {
            dbusReply = g_dbus_connection_call_sync(_dbusConnection, BLUEZ_INTERFACE, BLUEZ_OBJECT_HCI, BLUEZ_INTERFACE_ADAPTER, "StopDiscovery", nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &dbusError);
            if (!dbusReply) {
                TRACE(Trace::Error, ("Failed stopDiscovery dbus call. Error msg :  %s", dbusError->message));
                return false;
            }

            TRACE(Trace::Information, ("Stopped BT Scan"));
            if (g_main_is_running(_loop))
                g_main_loop_quit(_loop);

            _isScanning = false;
            return true;
        } else {
            TRACE(Trace::Error, ("Scanning is not started!"));
            return false;
        }
    }

    string BluetoothImplementation::DiscoveredDevices()
    {
        std::string deviceInfoList;

        _jsonDiscoveredDevices.ToString(deviceInfoList);
        return deviceInfoList;
    }

    bool BluetoothImplementation::Pair(string deviceId)
    {
        const auto& iterator = _discoveredDeviceIdMap.find(deviceId.c_str());
        if (iterator != _discoveredDeviceIdMap.end()) {
            TRACE(Trace::Information, ("Pairing BT Device. Device ID : [%s]", deviceId.c_str()));
            if (!PairDevice(iterator->second)) {
                TRACE(Trace::Error, ("Failed to Pair BT Device. Device ID : [%s]", deviceId.c_str()));
                return false;
            }
        }

        TRACE(Trace::Information, ("Paired BT Device. Device ID : [%s]", deviceId.c_str()));
        return true;
    }

    bool BluetoothImplementation::PairDevice(const string deviceId)
    {
        GError* dbusError = nullptr;
        GVariant* dbusReply = nullptr;

        // Turning 'Pairable' On.
        dbusReply = g_dbus_connection_call_sync(_dbusConnection, BLUEZ_INTERFACE, BLUEZ_OBJECT_HCI, DBUS_PROP_INTERFACE, "Set", g_variant_new("(ssv)",BLUEZ_INTERFACE_ADAPTER, "Pairable",g_variant_new("b", true)), nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &dbusError);
        if (!dbusReply) {
            TRACE(Trace::Error, ("Failed Pairable On dbus call. Error msg :  %s", dbusError->message));
            return false;
        }

        // Pairing wit BT Device.
        dbusReply = g_dbus_connection_call_sync(_dbusConnection, BLUEZ_INTERFACE, deviceId.c_str(), BLUEZ_INTERFACE_DEVICE, "Pair", nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &dbusError);
        if (!dbusReply) {
            std::size_t found = std::string(dbusError->message).find(BLUEZ_OBJECT_EXISTS);
            if (found != std::string::npos) {
                TRACE(Trace::Error, ("BT Device Already Paired. DBus msg : %s", dbusError->message));
                return true;
            }
            TRACE(Trace::Error, ("Failed Pair dbus call. Error msg :  %s", dbusError->message));
            return false;
        }

        return true;
    }

    bool BluetoothImplementation::GetPairedDevices()
    {
        GError* dbusError = nullptr;
        GVariant* dbusReply = nullptr;
        string deviceIdList[50];
        string deviceName;
        string deviceAddress;
        string devicePropertyType;
        string bluezDeviceName;
        bool paired;
        GVariantIter* iterator1;
        GVariantIter* iterator2;
        GVariantIter* iterator3;
        GVariant* devicePropertyValue;
        BTDeviceList::BTDeviceInfo pairedDeviceInfo;

        dbusReply = g_dbus_connection_call_sync(_dbusConnection, BLUEZ_INTERFACE, ROOT_OBJECT, DBUS_INTERFACE_OBJECT_MANAGER, "GetManagedObjects", nullptr, G_VARIANT_TYPE("(a{oa{sa{sv}}})"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &dbusError);
        if (!dbusReply) {
            TRACE(Trace::Error, ("Failed to Get Managed Objects. Error msg :  %s", dbusError->message));
            return false;
        }

        g_variant_get(dbusReply, "(a{oa{sa{sv}}})", &iterator1);

        for (int iterator = 0; g_variant_iter_next(iterator1, "{oa{sa{sv}}}", &deviceIdList[iterator], &iterator2); iterator++) {
            // Checking whether the object is controller or not.
            if (!strstr(deviceIdList[iterator].c_str(), DEVICE_ID))
                continue;

            TRACE(Trace::Information, ("Paired Device ID -  : [%s]", deviceIdList[iterator].c_str()));
            // Clearing previous data.
            deviceName = "";
            pairedDeviceInfo.Name = "";
            paired = false;
            while (g_variant_iter_loop(iterator2, "{sa{sv}}", &bluezDeviceName, &iterator3)) {
                while (g_variant_iter_loop(iterator3, "{sv}", &devicePropertyType, &devicePropertyValue)) {
                    if (strcmp(devicePropertyType.c_str(), "Address") == 0) {
                        g_variant_get(devicePropertyValue, "s", &deviceAddress);
                        pairedDeviceInfo.Address = deviceAddress;
                        TRACE(Trace::Information, ("Paired Device Address -  : [%s]", deviceAddress.c_str()));
                    }

                    if (strcmp(devicePropertyType.c_str(), "Name") == 0) {
                        g_variant_get(devicePropertyValue, "s", &deviceName);
                        pairedDeviceInfo.Name = deviceName;
                        TRACE(Trace::Information, ("Paired Device Name -  : [%s]", deviceName.c_str()));
                    }

                    if (strcmp(devicePropertyType.c_str(), "Paired") == 0)
                        g_variant_get(devicePropertyValue, "b", &paired);
                }
            }

            if (_pairedDeviceIdMap.find(deviceAddress.c_str()) == _pairedDeviceIdMap.end() && paired) {
                TRACE_L1("Added BT Device. Device ID : [%s]", deviceAddress.c_str());
                _pairedDeviceIdMap.insert(std::make_pair(deviceAddress.c_str(), deviceIdList[iterator].c_str()));
                _jsonPairedDevices.Add(pairedDeviceInfo);
            }
        }
        return true;
    }

    string BluetoothImplementation::PairedDevices()
    {
        std::string deviceInfoList;

        // Clearing previous data
        _jsonPairedDevices.Clear();
        _pairedDeviceIdMap.clear();
        if (GetPairedDevices())
            _jsonPairedDevices.ToString(deviceInfoList);
        else
            TRACE(Trace::Error, ("Failed to get Paired Device List"));

        return deviceInfoList;
    }

    bool BluetoothImplementation::Connect(string deviceId)
    {
        GError* dbusError = nullptr;
        GVariant* dbusReply = nullptr;

        const auto& iterator = _pairedDeviceIdMap.find(deviceId.c_str());
        if (iterator != _pairedDeviceIdMap.end()) {
            TRACE(Trace::Information, ("Connecting BT Device. Device ID : [%s]", deviceId.c_str()));
            dbusReply = g_dbus_connection_call_sync(_dbusConnection, BLUEZ_INTERFACE, iterator->second.c_str(), BLUEZ_INTERFACE_DEVICE, "Connect", nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &dbusError);
            if (!dbusReply) {
                TRACE(Trace::Error, ("Failed Connect dbus call. Error msg :  %s", dbusError->message));
                return false;
            }

            TRACE(Trace::Information, ("Connected BT Device. Device ID : [%s]", deviceId.c_str()));
            _connected = deviceId;
            return true;
        }

        TRACE(Trace::Error, ("Invalid BT Device ID. Device ID : [%s]", deviceId.c_str()));
        return false;

    }

    bool BluetoothImplementation::Disconnect()
    {
        GError* dbusError = nullptr;
        GVariant* dbusReply = nullptr;

        const auto& iterator = _pairedDeviceIdMap.find(_connected.c_str());
        if (iterator != _pairedDeviceIdMap.end()) {
            TRACE(Trace::Information, ("Disconnecting BT Device. Device ID : [%s]", _connected.c_str()));
            dbusReply = g_dbus_connection_call_sync(_dbusConnection, BLUEZ_INTERFACE, iterator->second.c_str(), BLUEZ_INTERFACE_DEVICE, "Disconnect", nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &dbusError);
            if (!dbusReply) {
                TRACE(Trace::Error, ("Failed Disconnect dbus call. Error msg :  %s", dbusError->message));
                return false;
            }

            TRACE(Trace::Information, ("Disconnected BT Device. Device ID : [%s]", _connected.c_str()));
            _connected = "";
            return true;
        }

        TRACE(Trace::Error, ("Invalid BT Device ID. Device ID : [%s]", _connected.c_str()));
        return false;
    }

    SERVICE_REGISTRATION(BluetoothImplementation, 1, 0);

}
} /* namespace WPEFramework::Plugin */
