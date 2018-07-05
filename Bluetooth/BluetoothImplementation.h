#include "Module.h"
#include "BluetoothJSONContainer.h"
#include <dbus/dbus.h>
#include <gio/gio.h>
#include <glib.h>
#include <interfaces/IBluetooth.h>
#include <map>
#include <unordered_set>

#define DBUS_INTERFACE_OBJECT_MANAGER   "org.freedesktop.DBus.ObjectManager"
#define DBUS_PROP_INTERFACE             "org.freedesktop.DBus.Properties"
#define BLUEZ_INTERFACE                 "org.bluez"
#define BLUEZ_OBJECT_HCI                "/org/bluez/hci0"
#define BLUEZ_INTERFACE_ADAPTER         "org.bluez.Adapter1"
#define BLUEZ_INTERFACE_DEVICE          "org.bluez.Device1"
#define BLUEZ_OBJECT_EXISTS             "org.bluez.Error.AlreadyExists"
#define ROOT_OBJECT                     "/"
#define DEVICE_ID                       "/org/bluez/hci0/dev_"
#define THREAD_EXIT_LIMIT               100

namespace WPEFramework {

namespace Plugin {

    class BluetoothImplementation : public Exchange::IBluetooth, public Core::Thread
    {
    private:
        BluetoothImplementation(const BluetoothImplementation&) = delete;
        BluetoothImplementation& operator=(const BluetoothImplementation&) = delete;

    public:
        BluetoothImplementation()
            : _isScanning(false)
            , _connected("")
        {
        }

        virtual ~BluetoothImplementation()
        {
            if (_loop) {
                if (g_main_loop_is_running(_loop)) {
                    UnsubscribeDBusSignals();
                    g_main_loop_quit(_loop);
                    g_main_context_pop_thread_default(_context);
                    g_main_loop_unref(_loop);
                    g_main_context_unref(_context);
                }
            }

            // Power off BT Adaptor.
            if (!PowerBTAdapter(false))
                TRACE(Trace::Error, ("Failed to switch on bluetooth adaptor"));

            if(_dbusConnection)
                g_object_unref(_dbusConnection);
        }

    public:

        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            if (!ConfigureBTAdapter()) {
                TRACE_L1("Failed configuring Bluetooth Adapter", __FUNCTION__);
                return -1;
            }
            return 0;
        }

        virtual uint32_t Worker()
        {
            _context = g_main_context_new();
            _loop = g_main_loop_new(_context, false);
            g_main_context_push_thread_default(_context);

            SubscribeDBusSignals();
            if (StartDiscovery())
                g_main_loop_run(_loop);

            UnsubscribeDBusSignals();
            g_main_context_pop_thread_default(_context);
            g_main_loop_unref(_loop);
            g_main_context_unref(_context);

            if (State() == RUNNING) {
                Block();
                Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED, THREAD_EXIT_LIMIT);
            }

            return (Core::infinite);
        }

    private:
        BEGIN_INTERFACE_MAP(BluetoothImplementation)
            INTERFACE_ENTRY(Exchange::IBluetooth)
        END_INTERFACE_MAP

    private:
        bool ConfigureBTAdapter();
        bool SubscribeDBusSignals();
        bool UnsubscribeDBusSignals();
        bool PowerBTAdapter(bool);
        bool Scan();
        bool StartDiscovery();
        bool StopScan();
        string DiscoveredDevices();
        bool GetPairedDevices();
        string PairedDevices();
        bool Connect(string);
        bool Disconnect();
        bool Pair(string);
        bool PairDevice(const string);
        static void InterfacesAdded(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant*, gpointer);
        static void InterfacesRemoved(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant*, gpointer);
        bool GetManagedObjects();
        bool IsScanning() { return _isScanning; };
        string Connected() { return _connected; };

    private:
        GMainLoop* _loop;
        GMainContext* _context;
        GDBusConnection* _dbusConnection;
        int _interfacesAdded;
        int _interfacesRemoved;
        bool _isScanning;
        string _connected;
        static std::map<string, string> _discoveredDeviceIdMap;
        static std::map<string, string> _pairedDeviceIdMap;
        static Core::JSON::ArrayType<BTDeviceList::BTDeviceInfo> _jsonDiscoveredDevices;
        static Core::JSON::ArrayType<BTDeviceList::BTDeviceInfo> _jsonPairedDevices;
    };

} /* namespace WPEFramework::Plugin */
}
