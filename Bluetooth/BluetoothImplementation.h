#include "Module.h"
#include "BluetoothJSONContainer.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/mgmt.h>
#include <gio/gio.h>
#include <glib.h>
#include <interfaces/IBluetooth.h>
#include <linux/uhid.h>
#include <linux/input.h>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <thread>
#include <unistd.h>
#include <unordered_set>

#define THREAD_EXIT_LIMIT               100
#define ADAPTER_INDEX                   0X00
#define HCI_DEVICE_ID                   0
#define ENABLE_MODE                     0x01
#define ONE_BYTE                        0X01
#define DATA_BUFFER_LENGTH              23
// SCAN PARAMETERS
#define SCAN_ENABLE                     0x01
#define SCAN_DISABLE                    0x00
#define SCAN_TYPE                       0x01
#define OWN_TYPE                        0X00
#define SCAN_TIMEOUT                    1000
#define SCAN_FILTER_POLICY              0x00
#define SCAN_INTERVAL                   0x0010
#define SCAN_WINDOW                     0x0010
#define SCAN_FILTER_DUPLICATES          1
#define EIR_NAME_SHORT                  0x08
#define EIR_NAME_COMPLETE               0x09
// L2CAP
#define LE_PUBLIC_ADDRESS               0x00
#define LE_RANDOM_ADDRESS               0x01
#define LE_ATT_CID                      4
// GATT
#define ATT_OP_ERROR                    0x01
#define ATT_OP_FIND_INFO_REQ            0x04
#define ATT_OP_FIND_INFO_RESP           0x05
#define ATT_OP_FIND_BY_TYPE_REQ         0x06
#define ATT_OP_FIND_BY_TYPE_RESP        0x07
#define ATT_OP_READ_BY_TYPE_REQ         0x08
#define ATT_OP_READ_BY_TYPE_RESP        0x09
#define ATT_OP_READ_REQ                 0x0A
#define ATT_OP_READ_BLOB_REQ            0x0C
#define ATT_OP_READ_BLOB_RESP           0x0D
#define ATT_OP_WRITE_REQ                0x12
#define ATT_OP_HANDLE_NOTIFY            0x1B
#define ATT_OP_WRITE_RESP               0x13
// UUID
#define PRIMARY_SERVICE_UUID            0x2800
#define HID_UUID                        0x1812
#define PNP_UUID                        0x2a50
#define DEVICE_NAME_UUID                0x2a00
#define REPORT_MAP_UUID                 0x2a4b
#define REPORT_UUID                     0x2a4d
#define UHID_PATH                       "/dev/uhid"

namespace WPEFramework {

namespace Plugin {

    class BluetoothImplementation : public Exchange::IBluetooth, public Core::Thread
    {
    private:
        BluetoothImplementation(const BluetoothImplementation&) = delete;
        BluetoothImplementation& operator=(const BluetoothImplementation&) = delete;

    public:
        BluetoothImplementation()
            : _scanning(false)
        {
        }

        virtual ~BluetoothImplementation()
        {
            // TODO
        }

    private:
        struct  handleRange {
            uint16_t startHandle;
            uint16_t endHandle;
        };

        struct  characteristicInfo {
            uint16_t handle;
            uint16_t offset;
        };

        struct deviceInformation {
            string name;
            uint16_t vendorID;
            uint16_t productID;
            uint16_t version;
            uint16_t country;
            std::vector<uint8_t> reportMap;
        };

        struct connectedDeviceInfo {
            string name;
            uint32_t l2capSocket;
            uint32_t connectionHandle;
            uint32_t uhidFD;
            bool connected;
        };

    public:
        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            if (!ConfigureBTAdapter()) {
                TRACE(Trace::Error,("Failed configuring Bluetooth Adapter"));
                return -1;
            }

            _hciHandle = hci_get_route(NULL);
            if (_hciHandle < 0) {
                TRACE(Trace::Error, ("Failed to get HCI Device ID"));
                return -1;
            }

            _hciSocket = hci_open_dev(_hciHandle);
            if (_hciSocket < 0) {
                TRACE(Trace::Error, ("Failed to Open HCI Socket"));
                return -1;
            }

            TRACE(Trace::Information, ("Configured Bluetooth Adapter"));
            return 0;
        }

        virtual uint32_t Worker()
        {
            // TODO
            return 0;
        }

    private:
        BEGIN_INTERFACE_MAP(BluetoothImplementation)
            INTERFACE_ENTRY(Exchange::IBluetooth)
        END_INTERFACE_MAP

    private:
        bool ConfigureBTAdapter();
        bool Scan();
        bool StopScan();
        void GetDeviceName(uint8_t*, ssize_t, char*);
        void ScanningThread();
        string DiscoveredDevices();
        string PairedDevices();
        bool Connect(string);
        bool Disconnect(string);
        bool Pair(string);
        bool IsScanning() { return _scanning; };
        string ConnectedDevices();
        bool CheckForHIDProfile(uint32_t);
        bool ReadHIDDeviceInformation(uint32_t, struct deviceInformation*);
        uint32_t CreateHIDDeviceNode(bdaddr_t, bdaddr_t, struct deviceInformation*);
        bool EnableInputReportNotification(uint32_t);
        void ReadingThread(string);
        bool SendCommand(uint32_t, uint8_t, handleRange, uint16_t, uint16_t, characteristicInfo, ssize_t, uint8_t*, uint8_t);
        bool PairDevice(string, uint32_t&, sockaddr_l2&);

    private:
        uint32_t _hciHandle;
        uint32_t _hciSocket;
        std::atomic<bool> _scanning;
        std::map<string, string> _discoveredDeviceIdMap;
        std::map<string, connectedDeviceInfo> _pairedDeviceInfoMap;
        std::map<string, string> _connectedDeviceInfoMap;
        Core::JSON::ArrayType<BTDeviceInfo> _jsonDiscoveredDevices;
        Core::JSON::ArrayType<BTDeviceInfo> _jsonPairedDevices;
        Core::JSON::ArrayType<BTDeviceInfo> _jsonConnectedDevices;
        Core::CriticalSection _scanningThreadLock;
        };

} /* namespace WPEFramework::Plugin */
}
