#ifndef __BLUETOOTH_JSON_CONTAINER_H
#define __BLUETOOTH_JSON_CONTAINER_H

#include "Module.h"
#include <core/core.h>

namespace WPEFramework {
namespace Plugin {

    class BTDeviceInfo : public Core::JSON::Container {
    private:
        BTDeviceInfo& operator=(const BTDeviceInfo&) = delete;

    public:
        BTDeviceInfo()
            : Core::JSON::Container()
              , Address()
              , Name()
        {
            Add(_T("address"), &Address);
            Add(_T("name"), &Name);
        }

        BTDeviceInfo(const BTDeviceInfo& copy)
            : Core::JSON::Container()
              , Address(copy.Address)
              , Name(copy.Name)
        {
            Add(_T("address"), &Address);
            Add(_T("name"), &Name);
        }

        ~BTDeviceInfo()
        {
        }

    public:
        Core::JSON::String Address;
        Core::JSON::String Name;
    };

    class BTStatus : public Core::JSON::Container {
    private:
        BTStatus(const BTStatus&) = delete;
        BTStatus& operator=(const BTStatus&) = delete;

    public:
        BTStatus()
            : Scanning()
            , DeviceList()
        {
            Add(_T("scanning"), &Scanning);
            Add(_T("deviceList"), &DeviceList);
        }

        virtual ~BTStatus()
        {
        }

    public:
        Core::JSON::Boolean Scanning;
        Core::JSON::ArrayType<BTDeviceInfo> DeviceList;
    };

} //namespace Plugin
} //namespace Solution

#endif // __BLUETOOTH_JSON_CONTAINER_H

