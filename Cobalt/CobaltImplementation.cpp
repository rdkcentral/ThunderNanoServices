/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "Module.h"
#include <interfaces/IMemory.h>
#include <interfaces/IBrowser.h>
#include <interfaces/IApplication.h>
#include <locale.h>

#include "third_party/starboard/wpe/shared/cobalt_api_wpe.h"

extern int StarboardMain(int argc, char **argv);

namespace WPEFramework {
namespace Plugin {

class CobaltImplementation:
        public Exchange::IBrowser,
        public Exchange::IApplication,
        public PluginHost::IStateControl {
public:
    enum connection {
        CABLE,
        WIRELESS
    };

private:
    class Config: public Core::JSON::Container {
    public:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        Config()
            : Core::JSON::Container()
            , Url()
            , Inspector()
            , Width(1280)
            , Height(720)
            , RepeatStart()
            , RepeatInterval()
            , ClientIdentifier()
            , ManufacturerName()
            , ChipsetModelNumber()
            , FirmwareVersion()
            , ModelName()
            , ModelYear()
            , OperatorName()
            , FriendlyName()
            , CertificationScope()
            , CertificationSecret()
            , Language()
            , Connection(CABLE)
            , PlaybackRates(true)
        {
            Add(_T("url"), &Url);
            Add(_T("inspector"), &Inspector);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);
            Add(_T("repeatstart"), &RepeatStart);
            Add(_T("repeatinterval"), &RepeatInterval);
            Add(_T("clientidentifier"), &ClientIdentifier);
            Add(_T("manufacturername"), &ManufacturerName);
            Add(_T("chipsetmodelnumber"), &ChipsetModelNumber);
            Add(_T("firmwareversion"), &FirmwareVersion);
            Add(_T("modelname"), &ModelName);
            Add(_T("modelyear"), &ModelYear);
            Add(_T("operatorname"), &OperatorName);
            Add(_T("friendlyname"), &FriendlyName);
            Add(_T("scope"), &CertificationScope);
            Add(_T("secret"), &CertificationSecret);
            Add(_T("language"), &Language);
            Add(_T("connection"), &Connection);
            Add(_T("playbackrates"), &PlaybackRates);
        }
        ~Config() {
        }

    public:
        Core::JSON::String Url;
        Core::JSON::String Inspector;
        Core::JSON::DecUInt16 Width;
        Core::JSON::DecUInt16 Height;
        Core::JSON::DecUInt32 RepeatStart;
        Core::JSON::DecUInt32 RepeatInterval;
        Core::JSON::String ClientIdentifier;
        Core::JSON::String ManufacturerName;
        Core::JSON::String ChipsetModelNumber;
        Core::JSON::String FirmwareVersion;
        Core::JSON::String ModelName;
        Core::JSON::String ModelYear;
        Core::JSON::String OperatorName;
        Core::JSON::String FriendlyName;
        Core::JSON::String CertificationScope;
        Core::JSON::String CertificationSecret;
        Core::JSON::String Language;
        Core::JSON::EnumType<connection> Connection;
        Core::JSON::Boolean PlaybackRates;
    };

    class NotificationSink: public Core::Thread {
    private:
        NotificationSink() = delete;
        NotificationSink(const NotificationSink&) = delete;
        NotificationSink& operator=(const NotificationSink&) = delete;

    public:
        NotificationSink(CobaltImplementation &parent) :
                _parent(parent), _waitTime(0), _command(
                        PluginHost::IStateControl::SUSPEND) {
        }
        virtual ~NotificationSink() {
            Stop();
            Wait(Thread::STOPPED | Thread::BLOCKED, Core::infinite);
        }

    public:
        void RequestForStateChange(
                const PluginHost::IStateControl::command command) {
            _command = command;
            Run();
        }

    private:
        virtual uint32_t Worker() {
            bool success = false;

            if ((IsRunning() == true) && (success == false)) {
                success = _parent.RequestForStateChange(_command);
            }
            Block();
            _parent.StateChangeCompleted(success, _command);
            return (Core::infinite);
        }

    private:
        CobaltImplementation &_parent;
        uint32_t _waitTime;
        PluginHost::IStateControl::command _command;
    };

    class CobaltWindow : public Core::Thread {
    private:
        CobaltWindow(const CobaltWindow&) = delete;
        CobaltWindow& operator=(const CobaltWindow&) = delete;

    public:
        CobaltWindow()
            : Core::Thread(0, _T("Cobalt"))
            , _url{"https://www.youtube.com/tv"}
            , _debugListenIp("0.0.0.0")
            , _debugPort()
        {
        }
        virtual ~CobaltWindow()
        {
            Block();
            Signal(SIGQUIT);
            Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
        }

        uint32_t Configure(PluginHost::IShell* service) {
            uint32_t result = Core::ERROR_NONE;

            Config config;
            config.FromString(service->ConfigLine());
            Core::SystemInfo::SetEnvironment(_T("HOME"), service->PersistentPath());
            Core::SystemInfo::SetEnvironment(_T("COBALT_TEMP"), service->VolatilePath());
            if (config.ClientIdentifier.IsSet() == true) {
                string value(service->Callsign() + ',' + config.ClientIdentifier.Value());
                Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), value);
            } else {
                Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), service->Callsign());
            }

            string width(Core::NumberType<uint16_t>(config.Width.Value()).Text());
            string height(Core::NumberType<uint16_t>(config.Height.Value()).Text());
            Core::SystemInfo::SetEnvironment(_T("COBALT_RESOLUTION_WIDTH"), width);
            Core::SystemInfo::SetEnvironment(_T("COBALT_RESOLUTION_HEIGHT"), height);

            if (width.empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_WIDTH"), width);
            }

            if (height.empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_HEIGHT"), height);
            }

            if (config.RepeatStart.IsSet() == true) {
                string repeatStart(Core::NumberType<uint32_t>(config.RepeatStart.Value()).Text());
                Core::SystemInfo::SetEnvironment(_T("COBALT_KEY_REPEAT_START"), repeatStart);
            }

            if (config.RepeatInterval.IsSet() == true) {
                string repeatInterval(Core::NumberType<uint32_t>(config.RepeatInterval.Value()).Text());
                Core::SystemInfo::SetEnvironment(_T("COBALT_KEY_REPEAT_INTERVAL"), repeatInterval);
            }

            if (config.ManufacturerName.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_MANUFACTURER_NAME"), config.ManufacturerName.Value());
            }

            if (config.ChipsetModelNumber.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_CHIPSET_MODEL_NUMBER"), config.ChipsetModelNumber.Value());
            }

            if (config.FirmwareVersion.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_FIRMWARE_VERSION"), config.FirmwareVersion.Value());
            }

            if (config.ModelName.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_MODEL_NAME"), config.ModelName.Value());
            }

            if (config.ModelYear.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_MODEL_YEAR"), config.ModelYear.Value());
            }

            if (config.OperatorName.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_OPERATOR_NAME"), config.OperatorName.Value());
            }

            if (config.FriendlyName.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_FRIENDLY_NAME"), config.FriendlyName.Value());
            }

            if (config.CertificationScope.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_CERTIFICATION_SCOPE"), config.CertificationScope.Value());
            }

            if (config.CertificationSecret.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_CERTIFICATION_SECRET"), config.CertificationSecret.Value());
            }

            if (config.Language.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("LANG"), config.Language.Value().c_str());
                Core::SystemInfo::SetEnvironment(_T("LANGUAGE"), config.Language.Value().c_str());
            }

            if ( (config.Connection.IsSet() == true) && (config.Connection == CobaltImplementation::connection::WIRELESS) ) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_CONNECTION_TYPE"), _T("wireless"));
            }

            if ( (config.PlaybackRates.IsSet() == true) && (config.PlaybackRates.Value() == false) ) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_SUPPORT_PLAYBACK_RATES"), _T("false"));
            }

            if (config.Url.IsSet() == true) {
              _url = config.Url.Value();
            }

            if (config.Inspector.Value().empty() == false) {
                string url(config.Inspector.Value());
                auto pos = url.find(":");
                if (pos != std::string::npos) {
                    _debugListenIp = url.substr(0, pos);
                    _debugPort = static_cast<uint16_t>(std::atoi(url.substr(pos + 1).c_str()));
                }
            }

            Run();
            return result;
        }

        bool Suspend(const bool suspend)
        {
            if (suspend == true) {
                third_party::starboard::wpe::shared::Suspend();
            }
            else {
                third_party::starboard::wpe::shared::Resume();
            }
            return (true);
        }

        string Url() const { return _url; }

    private:
        bool Initialize() override
        {
            sigset_t mask;
            sigemptyset(&mask);
            sigaddset(&mask, SIGQUIT);
            sigaddset(&mask, SIGUSR1);
            sigaddset(&mask, SIGCONT);
            pthread_sigmask(SIG_UNBLOCK, &mask, nullptr);
            return (true);
        }
        uint32_t Worker() override
        {
            const std::string cmdURL = "--url=" + _url;
            const std::string cmdDebugListenIp = "--dev_servers_listen_ip=" + _debugListenIp;
            const std::string cmdDebugPort = "--remote_debugging_port=" + std::to_string(_debugPort);
            const char* argv[] = {"Cobalt", cmdURL.c_str(), cmdDebugListenIp.c_str(), cmdDebugPort.c_str()};
            while (IsRunning() == true) {
                StarboardMain(4, const_cast<char**>(argv));
            }
            return (Core::infinite);
        }

        string _url;
        string _debugListenIp;
        uint16_t _debugPort;
    };

private:
    CobaltImplementation(const CobaltImplementation&) = delete;
    CobaltImplementation& operator=(const CobaltImplementation&) = delete;

public:
    CobaltImplementation() :
            _adminLock(),
            _state(PluginHost::IStateControl::UNINITIALIZED),
            _cobaltClients(),
            _stateControlClients(),
            _sink(*this) {
    }

    virtual ~CobaltImplementation() {
    }

    virtual uint32_t Configure(PluginHost::IShell *service) {
        uint32_t result = _window.Configure(service);
        _window.Suspend(true);
        _state = PluginHost::IStateControl::SUSPENDED;

        return (result);
    }

    virtual void SetURL(const string &URL) override {
        third_party::starboard::wpe::shared::SetURL(URL.c_str());
    }

    virtual string GetURL() const override {
        return _window.Url();
    }

    virtual uint32_t GetFPS() const override {
        return 0;
    }

    virtual void Hide(const bool hidden) {
    }

    virtual void Register(Exchange::IBrowser::INotification *sink) {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(
                std::find(_cobaltClients.begin(), _cobaltClients.end(), sink)
                        == _cobaltClients.end());

        _cobaltClients.push_back(sink);
        sink->AddRef();

        _adminLock.Unlock();
    }

    virtual void Unregister(Exchange::IBrowser::INotification *sink) {
        _adminLock.Lock();

        std::list<Exchange::IBrowser::INotification*>::iterator index(
                std::find(_cobaltClients.begin(), _cobaltClients.end(), sink));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _cobaltClients.end());

        if (index != _cobaltClients.end()) {
            (*index)->Release();
            _cobaltClients.erase(index);
        }

        _adminLock.Unlock();
    }

    virtual void Reset() { /*Not implemented yet!*/ }

    virtual void DeepLink(const string& deepLink) {
        third_party::starboard::wpe::shared::DeepLink(deepLink.c_str());
    }

    virtual void Register(PluginHost::IStateControl::INotification *sink) {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(
                std::find(_stateControlClients.begin(),
                        _stateControlClients.end(), sink)
                        == _stateControlClients.end());

        _stateControlClients.push_back(sink);
        sink->AddRef();

        _adminLock.Unlock();
    }

    virtual void Unregister(PluginHost::IStateControl::INotification *sink) {
        _adminLock.Lock();

        std::list<PluginHost::IStateControl::INotification*>::iterator index(
                std::find(_stateControlClients.begin(),
                        _stateControlClients.end(), sink));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _stateControlClients.end());

        if (index != _stateControlClients.end()) {
            (*index)->Release();
            _stateControlClients.erase(index);
        }

        _adminLock.Unlock();
    }

    virtual PluginHost::IStateControl::state State() const {
        return (_state);
    }

    virtual uint32_t Request(const PluginHost::IStateControl::command command) {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        _adminLock.Lock();
        if (_state == PluginHost::IStateControl::UNINITIALIZED) {
            // Seems we are passing state changes before we reached an operational Cobalt.
            // Just move the state to what we would like it to be :-)
            _state = (
                    command == PluginHost::IStateControl::SUSPEND ?
                            PluginHost::IStateControl::SUSPENDED :
                            PluginHost::IStateControl::RESUMED);
            result = Core::ERROR_NONE;
        } else {
            switch (command) {
            case PluginHost::IStateControl::SUSPEND:
                if (_state == PluginHost::IStateControl::RESUMED) {
                    _sink.RequestForStateChange(
                            PluginHost::IStateControl::SUSPEND);
                    result = Core::ERROR_NONE;
                }
                break;
            case PluginHost::IStateControl::RESUME:
                if (_state == PluginHost::IStateControl::SUSPENDED) {
                    _sink.RequestForStateChange(
                            PluginHost::IStateControl::RESUME);
                    result = Core::ERROR_NONE;
                }
                break;
            default:
                break;
            }
        }
        _adminLock.Unlock();

        return result;
    }

    void StateChangeCompleted(bool success,
            const PluginHost::IStateControl::command request) {
        if (success) {
            switch (request) {
            case PluginHost::IStateControl::RESUME:

                _adminLock.Lock();

                if (_state != PluginHost::IStateControl::RESUMED) {
                    StateChange(PluginHost::IStateControl::RESUMED);
                }

                _adminLock.Unlock();
                break;
            case PluginHost::IStateControl::SUSPEND:

                _adminLock.Lock();

                if (_state != PluginHost::IStateControl::SUSPENDED) {
                    StateChange(PluginHost::IStateControl::SUSPENDED);
                }

                _adminLock.Unlock();
                break;
            default:
                ASSERT(false);
                break;
            }
        } else {
            StateChange(PluginHost::IStateControl::EXITED);
        }
    }

    BEGIN_INTERFACE_MAP (CobaltImplementation)
        INTERFACE_ENTRY (Exchange::IBrowser)
        INTERFACE_ENTRY (PluginHost::IStateControl)
        INTERFACE_ENTRY (Exchange::IApplication)
    END_INTERFACE_MAP

private:
    inline bool RequestForStateChange(
            const PluginHost::IStateControl::command command) {
        bool result = false;

        switch (command) {
            case PluginHost::IStateControl::SUSPEND: {
                if (_window.Suspend(true) == true) {
                    result = true;
                }
                break;
            }
            case PluginHost::IStateControl::RESUME: {
                if (_window.Suspend(false) == true) {
                    result = true;
                }
                break;
            }
            default:
                ASSERT(false);
                break;
        }
        return result;
    }

    void StateChange(const PluginHost::IStateControl::state newState) {
        _adminLock.Lock();

        _state = newState;

        std::list<PluginHost::IStateControl::INotification*>::iterator index(
                _stateControlClients.begin());

        while (index != _stateControlClients.end()) {
            (*index)->StateChange(newState);
            index++;
        }

        _adminLock.Unlock();
    }

private:
    CobaltWindow _window;
    mutable Core::CriticalSection _adminLock;
    PluginHost::IStateControl::state _state;
    std::list<Exchange::IBrowser::INotification*> _cobaltClients;
    std::list<PluginHost::IStateControl::INotification*> _stateControlClients;
    NotificationSink _sink;
};

SERVICE_REGISTRATION(CobaltImplementation, 1, 0);

}
/* namespace Plugin */

namespace Cobalt {

class MemoryObserverImpl: public Exchange::IMemory {
private:
    MemoryObserverImpl();
    MemoryObserverImpl(const MemoryObserverImpl&);
    MemoryObserverImpl& operator=(const MemoryObserverImpl&);

    public:
    MemoryObserverImpl(const RPC::IRemoteConnection* connection) :
        _main(connection == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId()) {
    }
    ~MemoryObserverImpl() {
    }

    public:
    virtual uint64_t Resident() const {
        return _main.Resident();
    }
    virtual uint64_t Allocated() const {
        return _main.Allocated();
    }
    virtual uint64_t Shared() const {
        return _main.Shared();
    }
    virtual uint8_t Processes() const {
        return (IsOperational() ? 1 : 0);
    }

    virtual const bool IsOperational() const {
        return _main.IsActive();
    }

    BEGIN_INTERFACE_MAP (MemoryObserverImpl)INTERFACE_ENTRY (Exchange::IMemory)END_INTERFACE_MAP

private:
    Core::ProcessInfo _main;
    };

    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection) {
        ASSERT(connection != nullptr);
        Exchange::IMemory* result = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);
        return (result);
    }
}


ENUM_CONVERSION_BEGIN(Plugin::CobaltImplementation::connection)

    { Plugin::CobaltImplementation::connection::CABLE,    _TXT("cable")    },
    { Plugin::CobaltImplementation::connection::WIRELESS, _TXT("wireless") },

ENUM_CONVERSION_END(Plugin::CobaltImplementation::connection)

} // namespace
