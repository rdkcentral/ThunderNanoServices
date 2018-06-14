#ifndef __BLUETOOTH_JSON_CONTAINER_H
#define __BLUETOOTH_JSON_CONTAINER_H

#include "Module.h"
#include <core/core.h>

namespace WPEFramework {
namespace Plugin {

    class BTDeviceList : public Core::JSON::Container {
    private:
        BTDeviceList(const BTDeviceList&) = delete;
        BTDeviceList& operator=(const BTDeviceList&) = delete;

    public:
        class BTDeviceInfo : public Core::JSON::Container {
        private:
            BTDeviceInfo& operator=(const BTDeviceInfo&) = delete;

        public:
            BTDeviceInfo()
                : Core::JSON::Container()
                , Address()
                , Name()
            {
                Add(_T("Address"), &Address);
                Add(_T("Name"), &Name);
            }

            BTDeviceInfo(const BTDeviceInfo& copy)
                : Core::JSON::Container()
                , Address(copy.Address)
                , Name(copy.Name)
            {
                Add(_T("Address"), &Address);
                Add(_T("Name"), &Name);
            }

            ~BTDeviceInfo()
            {
            }
        public:
            Core::JSON::String Address;
            Core::JSON::String Name;
        };

    public:
        BTDeviceList()
            : Core::JSON::Container()
            , DeviceInfoList()
            , DeviceId()
        {
            Add(_T("DeviceInfoList"), &DeviceInfoList);
            Add(_T("DeviceId"), &DeviceId);
        }
        ~BTDeviceList()
        {
        }
    public:
        Core::JSON::ArrayType<BTDeviceInfo> DeviceInfoList;
        //BTDeviceInfo DeviceInfoList;
        Core::JSON::String DeviceId;
    };

} //namespace Plugin
} //namespace Solution

#endif // __BLUETOOTH_JSON_CONTAINER_H

