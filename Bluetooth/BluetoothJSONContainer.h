#ifndef __BLUETOOTH_JSON_CONTAINER_H
#define __BLUETOOTH_JSON_CONTAINER_H

#include "Module.h"
#include <core/core.h>

namespace WPEFramework {
namespace Plugin {

    class BTStatus : public Core::JSON::Container {
    private:
        BTStatus(const BTStatus&) = delete;
        BTStatus& operator=(const BTStatus&) = delete;

    public:
        BTStatus()
            : Scanning()
            , Connected()
        {
            Add(_T("scanning"), &Scanning);
            Add(_T("connected"), &Connected);
        }

        virtual ~BTStatus()
        {
        }

    public:
        Core::JSON::Boolean Scanning;
        Core::JSON::String Connected;
    };

    class BTDeviceList : public Core::JSON::Container {
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

    private:
        BTDeviceList(const BTDeviceList&) = delete;
        BTDeviceList& operator=(const BTDeviceList&) = delete;

    public:
        BTDeviceList()
            : Core::JSON::Container()
            , DeviceList()
        {
            Add(_T("DeviceList"), &DeviceList);
        }

        ~BTDeviceList()
        {
        }

    public:
        Core::JSON::ArrayType<BTDeviceInfo> DeviceList;
    };

} //namespace Plugin
} //namespace Solution

#endif // __BLUETOOTH_JSON_CONTAINER_H

