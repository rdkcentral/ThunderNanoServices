/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

namespace Thunder {
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
    class Config : public Core::JSON::Container {
    public:
        class Window : public Core::JSON::Container {
        public:
            Window& operator=(Window&&) = delete;
            Window& operator=(const Window&) = delete;

            Window()
                : Core::JSON::Container()
                , Width(1280)
                , Height(720) {
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
            }
            Window(Window&& move) 
                : Core::JSON::Container()
                , Width(std::move(move.Width))
                , Height(std::move(move.Height)) {
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
            }
            Window(const Window& copy) 
                : Core::JSON::Container()
                , Width(copy.Width)
                , Height(copy.Height) {
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
            }
            ~Window() override = default;

        public:
            Core::JSON::DecUInt16 Width;
            Core::JSON::DecUInt16 Height;
        };

        Config(Config&&) = delete;
        Config(const Config&) = delete;
        Config& operator=(Config&&) = delete;
        Config& operator=(const Config&) = delete;

        Config()
            : Core::JSON::Container()
            , Url()
            , LogLevel()
            , Inspector()
            , Graphics()
            , Video()
            , RepeatStart()
            , RepeatInterval()
            , ClientIdentifier()
            , OperatorName()
            , CertificationScope()
            , CertificationSecret()
            , Language()
            , Connection(CABLE)
            , PlaybackRates(true)
            , FrameRate(30)
            , AudioBufferBudget(5)
            , VideoBufferBudget(300)
            , ProgressiveBufferBudget(12)
            , MediaGarbageCollect(170)
        {
            Add(_T("url"), &Url);
            Add(_T("loglevel"), &LogLevel);
            Add(_T("inspector"), &Inspector);
            Add(_T("graphics"), &Graphics);
            Add(_T("video"), &Video);
            Add(_T("repeatstart"), &RepeatStart);
            Add(_T("repeatinterval"), &RepeatInterval);
            Add(_T("clientidentifier"), &ClientIdentifier);
            Add(_T("operatorname"), &OperatorName);
            Add(_T("scope"), &CertificationScope);
            Add(_T("secret"), &CertificationSecret);
            Add(_T("language"), &Language);
            Add(_T("connection"), &Connection);
            Add(_T("playbackrates"), &PlaybackRates);
            Add(_T("framerate"), &FrameRate);
            Add(_T("audiobufferbudget"), &AudioBufferBudget);
            Add(_T("videobufferbudget"), &VideoBufferBudget);
            Add(_T("progressivebufferbudget"), &ProgressiveBufferBudget);
            Add(_T("mediagarbagecollect"), &MediaGarbageCollect);
        }
        ~Config() override = default;

    public:
        Core::JSON::String Url;
        Core::JSON::String LogLevel;
        Core::JSON::String Inspector;
        Window Graphics;
        Window Video;
        Core::JSON::DecUInt32 RepeatStart;
        Core::JSON::DecUInt32 RepeatInterval;
        Core::JSON::String ClientIdentifier;
        Core::JSON::String OperatorName;
        Core::JSON::String CertificationScope;
        Core::JSON::String CertificationSecret;
        Core::JSON::String Language;
        Core::JSON::EnumType<connection> Connection;
        Core::JSON::Boolean PlaybackRates;
        Core::JSON::DecUInt8 FrameRate;
        Core::JSON::DecUInt16 AudioBufferBudget;
        Core::JSON::DecUInt16 VideoBufferBudget;
        Core::JSON::DecUInt16 ProgressiveBufferBudget;
        Core::JSON::DecUInt16 MediaGarbageCollect;
    };

    class NotificationSink: public Core::Thread {
    private:
        NotificationSink() = delete;
        NotificationSink(const NotificationSink&) = delete;
        NotificationSink& operator=(const NotificationSink&) = delete;

    public:
        NotificationSink(CobaltImplementation &parent)
            : _parent(parent)
            , _waitTime(0)
            , _command(PluginHost::IStateControl::SUSPEND)
        {
        }
        ~NotificationSink() override
        {
            Stop();
            Wait(Thread::STOPPED | Thread::BLOCKED, Core::infinite);
        }

    public:
        void RequestForStateChange(
                const PluginHost::IStateControl::command command)
        {
            _command = command;
            Run();
        }

    private:
        uint32_t Worker() override
        {
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
        CobaltWindow(CobaltImplementation &parent)
            : Core::Thread(0, _T("Cobalt"))
            , _parent(parent)
            , _url{"https://www.youtube.com/tv"}
            , _logLevel("info")
            , _language()
            , _debugListenIp("0.0.0.0")
            , _debugPort()
        {
        }
        ~CobaltWindow() override
        {
            Block();
            third_party::starboard::wpe::shared::Stop();
            Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
        }

        uint32_t Configure(PluginHost::IShell* service)
        {
            bool bufferSet = false;
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

            string width(Core::NumberType<uint16_t>(config.Graphics.Width.Value()).Text());
            string height(Core::NumberType<uint16_t>(config.Graphics.Height.Value()).Text());
            Core::SystemInfo::SetEnvironment(_T("COBALT_RESOLUTION_WIDTH"), width);
            Core::SystemInfo::SetEnvironment(_T("COBALT_RESOLUTION_HEIGHT"), height);

            if (config.Video.IsSet() == false) {
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_WIDTH"), width);
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_HEIGHT"), height);
            }
            else {
                string videoWidth(Core::NumberType<uint16_t>(config.Video.Width.Value()).Text());
                string videoHeight(Core::NumberType<uint16_t>(config.Video.Height.Value()).Text());
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_WIDTH"), videoWidth);
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_HEIGHT"), videoHeight);
            }

            if (config.FrameRate.IsSet() == true) {
                string rate (Core::NumberType<uint16_t>(config.FrameRate.Value()).Text());
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_MAX_FRAMERATE"), rate);
            }

            if (config.RepeatStart.IsSet() == true) {
                string repeatStart(Core::NumberType<uint32_t>(config.RepeatStart.Value()).Text());
                Core::SystemInfo::SetEnvironment(_T("COBALT_KEY_REPEAT_START"), repeatStart);
            }

            if (config.RepeatInterval.IsSet() == true) {
                string repeatInterval(Core::NumberType<uint32_t>(config.RepeatInterval.Value()).Text());
                Core::SystemInfo::SetEnvironment(_T("COBALT_KEY_REPEAT_INTERVAL"), repeatInterval);
            }

            if (config.OperatorName.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_OPERATOR_NAME"), config.OperatorName.Value());
            }

            if (config.CertificationScope.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_CERTIFICATION_SCOPE"), config.CertificationScope.Value());
            }

            if (config.CertificationSecret.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_CERTIFICATION_SECRET"), config.CertificationSecret.Value());
            }

            if (config.AudioBufferBudget.IsSet() == true) {
                bufferSet = true;
                Core::SystemInfo::SetEnvironment(_T("COBALT_MEDIA_AUDIO_BUFFER_BUDGET"), Core::NumberType<uint16_t>(config.AudioBufferBudget.Value()).Text());
            }

            if (config.VideoBufferBudget.IsSet() == true) {
                bufferSet = true;
                Core::SystemInfo::SetEnvironment(_T("COBALT_MEDIA_VIDEO_BUFFER_BUDGET"), Core::NumberType<uint16_t>(config.VideoBufferBudget.Value()).Text());
            }

            if (config.MediaGarbageCollect.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("COBALT_MEDIA_GARBAGE_COLLECT"), Core::NumberType<uint16_t>(config.MediaGarbageCollect.Value()).Text());
            }

            if (config.ProgressiveBufferBudget.IsSet() == true) {
                bufferSet = true;
                Core::SystemInfo::SetEnvironment(_T("COBALT_MEDIA_PROGRESSIVE_BUFFER_BUDGET"), Core::NumberType<uint16_t>(config.ProgressiveBufferBudget.Value()).Text());
            }

            if (bufferSet == true) {
                uint32_t value = config.ProgressiveBufferBudget.Value() + config.VideoBufferBudget.Value() + config.AudioBufferBudget.Value();
                Core::SystemInfo::SetEnvironment(_T("COBALT_MEDIA_MAX_BUFFER_BUDGET"), Core::NumberType<uint16_t>(value));
            }

            if (config.Language.IsSet() == true) {
                Language(config.Language.Value());
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

            if (config.LogLevel.IsSet() == true) {
                _logLevel = config.LogLevel.Value();
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

        inline string Url() const { return _url; }
        inline void Language(string& language) const { language = _language; }
        inline void Language(const string& language)
        {
            Core::SystemInfo::SetEnvironment(_T("LANG"), language.c_str());
            Core::SystemInfo::SetEnvironment(_T("LANGUAGE"), language.c_str());
            _language = language;
        }

    private:
        uint32_t Worker() override
        {
            const std::string cmdURL = "--url=" + _url;
            const std::string cmdDebugListenIp = "--dev_servers_listen_ip=" + _debugListenIp;
            const std::string cmdDebugPort = "--remote_debugging_port=" + std::to_string(_debugPort);
            const std::string cmdLogLevel = "--min_log_level=" + _logLevel;
            const char* argv[] = {"Cobalt", cmdURL.c_str(), cmdDebugListenIp.c_str(), cmdDebugPort.c_str(), cmdLogLevel.c_str()};
            if (IsRunning() == true) {
                StarboardMain(5, const_cast<char**>(argv));
            }
            Block();
            // Do plugin de-activation
            _parent.StateChangeCompleted(false, static_cast<PluginHost::IStateControl::command>(~0));
            return (Core::infinite);
        }

        CobaltImplementation &_parent;
        string _url;
        string _logLevel;
        string _language;
        string _debugListenIp;
        uint16_t _debugPort;
    };

private:
    CobaltImplementation(const CobaltImplementation&) = delete;
    CobaltImplementation& operator=(const CobaltImplementation&) = delete;

public:
    CobaltImplementation()
        : _language()
        , _window(*this)
        , _adminLock()
        , _state(PluginHost::IStateControl::UNINITIALIZED)
        , _cobaltBrowserClients()
        , _stateControlClients()
        , _sink(*this)
        , _service(nullptr)
    {
    }

    ~CobaltImplementation() override
    {
        if (_service) {
            _service->Release();
            _service = nullptr;
        }
    }

    uint32_t Configure(PluginHost::IShell *service) override
    {
        uint32_t result = _window.Configure(service);
        _window.Suspend(!service->Resumed());
        _state = (!service->Resumed()) ? PluginHost::IStateControl::SUSPENDED : PluginHost::IStateControl::RESUMED;

        _service = service;
        if (_service) {
            _service->AddRef();
        }

        return (result);
    }

    void SetURL(const string &URL) override
    {
        third_party::starboard::wpe::shared::SetURL(URL.c_str());
    }

    string GetURL() const override
    {
        return _window.Url();
    }

    uint32_t GetFPS() const override
    {
        return 0;
    }

    void Hide(VARIABLE_IS_NOT_USED const bool hidden) override
    {
    }

    void Register(Exchange::IBrowser::INotification *sink) override
    {
        ASSERT(sink != nullptr);

        _adminLock.Lock();

        std::list<Exchange::IBrowser::INotification*>::iterator index(
                std::find(_cobaltBrowserClients.begin(), _cobaltBrowserClients.end(), sink));

        // Make sure a sink is not registered multiple times.
        ASSERT(index == _cobaltBrowserClients.end());

        if (index == _cobaltBrowserClients.end()) {
            _cobaltBrowserClients.push_back(sink);
            sink->AddRef();
        }

        _adminLock.Unlock();
    }

    void Unregister(Exchange::IBrowser::INotification *sink)  override
    {
        ASSERT(sink != nullptr);

        _adminLock.Lock();

        std::list<Exchange::IBrowser::INotification*>::iterator index(
                std::find(_cobaltBrowserClients.begin(), _cobaltBrowserClients.end(), sink));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _cobaltBrowserClients.end());

        if (index != _cobaltBrowserClients.end()) {
            (*index)->Release();
            _cobaltBrowserClients.erase(index);
        }

        _adminLock.Unlock();
    }

    void Register(VARIABLE_IS_NOT_USED Exchange::IApplication::INotification* sink) override
    {
        // Kept empty since visibility change is not supported
    }

    void Unregister(VARIABLE_IS_NOT_USED Exchange::IApplication::INotification* sink) override
    {
        // Kept empty since visibility change is not supported
    }

    uint32_t Reset(const resettype type) override
    {
        uint32_t status = Core::ERROR_GENERAL;
        switch (type) {
        case FACTORY:
            if (third_party::starboard::wpe::shared::Reset(third_party::starboard::wpe::shared::ResetType::kFactory) == true) {
                status = Core::ERROR_NONE;
            }
            break;
        case CACHE:
            if (third_party::starboard::wpe::shared::Reset(third_party::starboard::wpe::shared::ResetType::kCache) == true) {
                status = Core::ERROR_NONE;
            }
            break;
        case CREDENTIALS:
            if (third_party::starboard::wpe::shared::Reset(third_party::starboard::wpe::shared::ResetType::kCredentials) == true) {
                status = Core::ERROR_NONE;
            }
            break;
        default:
            status = Core::ERROR_NOT_SUPPORTED;
            break;
        }

        return status;
    }

    uint32_t Identifier(string& id) const override
    {
        PluginHost::ISubSystem* subSystem = _service->SubSystems();
        if (subSystem != nullptr) {

            const PluginHost::ISubSystem::IIdentifier* identifier(subSystem->Get<PluginHost::ISubSystem::IIdentifier>());
            if (identifier != nullptr) {
                uint8_t buffer[64];

                buffer[0] = static_cast<const PluginHost::ISubSystem::IIdentifier*>(identifier)
                            ->Identifier(sizeof(buffer) - 1, &(buffer[1]));

                if (buffer[0] != 0) {
                    id = Core::SystemInfo::Instance().Id(buffer, ~0);
                }

                identifier->Release();
            }
            subSystem->Release();
        }

        return Core::ERROR_NONE;
    }

    uint32_t ContentLink(const string& link) override
    {
        third_party::starboard::wpe::shared::DeepLink(link.c_str());
        return Core::ERROR_NONE;
    }

    uint32_t LaunchPoint(VARIABLE_IS_NOT_USED launchpointtype& point) const override
    {
        return Core::ERROR_UNAVAILABLE;
    }

    uint32_t LaunchPoint(VARIABLE_IS_NOT_USED const launchpointtype& point) override
    {
        return Core::ERROR_UNAVAILABLE;
    }

    uint32_t Visible(VARIABLE_IS_NOT_USED bool& visiblity) const override
    {
        return Core::ERROR_UNAVAILABLE;
    }

    uint32_t Visible(VARIABLE_IS_NOT_USED const bool visiblity) override
    {
        return Core::ERROR_UNAVAILABLE;
    }

    uint32_t Language(string& language) const override
    {
        _window.Language(language);
        return Core::ERROR_NONE;
    }

    uint32_t Language(const string& language) override
    {
        _window.Language(language);
        return Core::ERROR_NONE;
    }

    void Register(PluginHost::IStateControl::INotification *sink) override
    {
        ASSERT(sink != nullptr);

        _adminLock.Lock();

        std::list<PluginHost::IStateControl::INotification*>::iterator index(
                std::find(_stateControlClients.begin(),
                        _stateControlClients.end(), sink));

        // Make sure a sink is not registered multiple times.
        ASSERT(index == _stateControlClients.end());

        if (index == _stateControlClients.end()) {
            _stateControlClients.push_back(sink);
            sink->AddRef();
        }

        _adminLock.Unlock();
    }

    void Unregister(PluginHost::IStateControl::INotification *sink) override
    {
        ASSERT(sink != nullptr);

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

    PluginHost::IStateControl::state State() const override
    {
        return (_state);
    }

    uint32_t Request(const PluginHost::IStateControl::command command) override
    {
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
            const PluginHost::IStateControl::command command)
    {
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

    void StateChange(const PluginHost::IStateControl::state newState)
    {
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
    string _language;
    CobaltWindow _window;
    mutable Core::CriticalSection _adminLock;
    PluginHost::IStateControl::state _state;
    std::list<Exchange::IBrowser::INotification*> _cobaltBrowserClients;
    std::list<PluginHost::IStateControl::INotification*> _stateControlClients;
    NotificationSink _sink;
    PluginHost::IShell* _service;
};

SERVICE_REGISTRATION(CobaltImplementation, 1, 0)

}
/* namespace Plugin */

ENUM_CONVERSION_BEGIN(Plugin::CobaltImplementation::connection)

    { Plugin::CobaltImplementation::connection::CABLE,    _TXT("cable")    },
    { Plugin::CobaltImplementation::connection::WIRELESS, _TXT("wireless") },

ENUM_CONVERSION_END(Plugin::CobaltImplementation::connection)

} // namespace
