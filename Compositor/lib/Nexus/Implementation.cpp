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

#ifndef NEXUS_SERVER_EXTERNAL
#include "NexusServer/NexusServer.h"
#else
#include <nexus_config.h>
#include <nexus_types.h>
#include <nxclient.h>
#endif

#include "NexusServer/Settings.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {

    /* -------------------------------------------------------------------------------------------------------------
     *
     * ------------------------------------------------------------------------------------------------------------- */
    class CompositorImplementation : public Exchange::IComposition {
    private:
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , HardwareDelay(0)
                , Resolution(Exchange::IComposition::ScreenResolution::ScreenResolution_720p)
                , AllowedClients()
            {
                Add(_T("hardwareready"), &HardwareDelay);
                Add(_T("resolution"), &Resolution);
                Add(_T("allowedclients"), &AllowedClients);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt8 HardwareDelay;
            Core::JSON::EnumType<Exchange::IComposition::ScreenResolution> Resolution;
            Core::JSON::ArrayType<Core::JSON::String> AllowedClients;
        };

        class Postpone : public Core::Thread {
        private:
            Postpone() = delete;
            Postpone(const Postpone&) = delete;
            Postpone& operator=(const Postpone&) = delete;

        public:
            Postpone(CompositorImplementation& parent, const uint16_t delay)
                : _parent(parent)
                , _trigger(false, true)
                , _delay(delay * 1000)
            {
            }
            virtual ~Postpone()
            {
                Block();
                _trigger.SetEvent();
            }

        public:
            uint32_t Worker()
            {
                if (_trigger.Lock(_delay) == Core::ERROR_TIMEDOUT) {
                    _parent.PlatformReady();
                }
                Stop();
                return (Core::infinite);
            }

        private:
            CompositorImplementation& _parent;
            Core::Event _trigger;
            uint32_t _delay;
        };


#ifndef NEXUS_SERVER_EXTERNAL
        class Sink
            : public Broadcom::Platform::IClient,
              public Broadcom::Platform::IStateChange {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

        public:
            explicit Sink(CompositorImplementation* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Sink()
            {
            }

        public:
            //   Broadcom::Platform::ICallback methods
            // -------------------------------------------------------------------------------------------------------
            /* virtual */ void Attached(Exchange::IComposition::IClient* client)
            {
                _parent.Add(client);
            }

            /* virtual */ void Detached(const char clientName[])
            {
                _parent.Remove(clientName);
            }

            /* virtual */ virtual void StateChange(Broadcom::Platform::server_state state)
            {
                if (state == Broadcom::Platform::OPERATIONAL) {
                    _parent.StateChange();
                }
            }

        private:
            inline void PlatformReady()
            {
                _parent.PlatformReady();
            }

        private:
            CompositorImplementation& _parent;
        };
#endif
    public:
        CompositorImplementation()
            : _adminLock()
            , _service(nullptr)
            , _observers()
            , _clients()
            , _allowedClients()
            , _joined(false)
            , _joinSettings()
            , _displayFormat()
            , _delay(nullptr)
#ifndef NEXUS_SERVER_EXTERNAL
            , _sink(this)
            , _nxserver(nullptr)
#endif
        {
        }

        ~CompositorImplementation()
        {
            if (_joined == true) {
                NxClient_Uninit();
            }
            if (_delay != nullptr) {
                delete _delay;
            }
#ifndef NEXUS_SERVER_EXTERNAL
            if (_nxserver != nullptr) {
                delete _nxserver;
            }
#endif
            if (_service != nullptr) {
                _service->Release();
            }
        }

    public:
        BEGIN_INTERFACE_MAP(CompositorImplementation)
            INTERFACE_ENTRY(Exchange::IComposition)
        END_INTERFACE_MAP

    public:
        // -------------------------------------------------------------------------------------------------------
        //   IComposition methods
        // -------------------------------------------------------------------------------------------------------
        /* virtual */ uint32_t Configure(PluginHost::IShell* service)
        {
            string configuration(service->ConfigLine());

            _service = service;
            _service->AddRef();

            NxClient_GetDefaultJoinSettings(&(_joinSettings));
            strcpy(_joinSettings.name, service->Callsign().c_str());

            Config info;
            info.FromString(configuration);

            auto index(info.AllowedClients.Elements());
            while (index.Next() == true) {
                _allowedClients.emplace_back(index.Current().Value());
            }

            HardwareDelay(info.HardwareDelay.Value(), info.Resolution.Value());

#ifndef NEXUS_SERVER_EXTERNAL
            ASSERT(_nxserver == nullptr);

            NEXUS_VideoFormat format = NEXUS_VideoFormat_eUnknown;
            if (info.Resolution.IsSet() == true) {
                const auto index(formatLookup.find(info.Resolution.Value()));

                if ((index != formatLookup.cend()) && (index->second != NEXUS_VideoFormat_eUnknown)) {
                    format = index->second;
                }
            }

            _nxserver = new Broadcom::Platform(&_sink, &_sink, configuration, format);

            ASSERT(_nxserver != nullptr);

            if ((_nxserver != nullptr) && (_nxserver->State() == Broadcom::Platform::OPERATIONAL) && (Join() == false)) {
                TRACE(Trace::Information, (_T("Could not Join the started NXServer.")));
            }
#else
            StateChange();
            if (Join() == false) {
                TRACE(Trace::Information, (_T("Could not Join the started NXServer.")));
            }
#endif
            return Core::ERROR_NONE;
        }

        /* virtual */ void Register(Exchange::IComposition::INotification* notification)
        {
            // Do not double register a notification sink.
            _adminLock.Lock();

            ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());

            notification->AddRef();

            _observers.push_back(notification);

            std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());

            while (index != _clients.end()) {

                notification->Attached((*index)->Name(), *index);
                index++;
            }

            _adminLock.Unlock();
        }

        /* virtual */ void Unregister(Exchange::IComposition::INotification* notification)
        {
            _adminLock.Lock();

            std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

            // Do not un-register something you did not register.
            ASSERT(index != _observers.end());

            if (index != _observers.end()) {

                _observers.erase(index);

                notification->Release();
            }

            _adminLock.Unlock();
        }

        /* virtual */ uint32_t Resolution(const Exchange::IComposition::ScreenResolution format) override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_joined == true) {

                NxClient_DisplaySettings displaySettings;

                NxClient_GetDisplaySettings(&displaySettings);

                const auto index(formatLookup.find(format));

                if ((index != formatLookup.cend()) && (index->second != NEXUS_VideoFormat_eUnknown)) {

                    if (index->second != displaySettings.format) {

                        displaySettings.format = index->second;
                        if (NxClient_SetDisplaySettings(&displaySettings) != 0) {
                            TRACE(Trace::Information, (_T("Error in setting NxClient DisplaySettings")));
                            result = Core::ERROR_GENERAL;
                        }
                        else {
                            result = Core::ERROR_NONE;
                        }
                    }
                } else {
                    result = ERROR_UNKNOWN_KEY;
                }
            }
            return (result);
        }

        /* virtual */ Exchange::IComposition::ScreenResolution Resolution() const override
        {
            Exchange::IComposition::ScreenResolution result(Exchange::IComposition::ScreenResolution_Unknown);

            if (_joined == true) {
                NxClient_DisplaySettings displaySettings;

                NxClient_GetDisplaySettings(&displaySettings);
                NEXUS_VideoFormat format = displaySettings.format;
                const auto index = std::find_if(formatLookup.cbegin(), formatLookup.cend(),
                    [format](const std::pair<const Exchange::IComposition::ScreenResolution, const NEXUS_VideoFormat>& entry) { return entry.second == format; });

                if (index != formatLookup.cend()) {
                    result = index->first;
                }
            }
            return (result);
        }

        inline void StateChange()
        {
            if (_delay != nullptr) {
                _delay->Run();
            } else {
                PlatformReady();
            }
        }

    private:
        void Add(Exchange::IComposition::IClient* client)
        {
            ASSERT(client != nullptr);

            if (client != nullptr) {

                const std::string name(client->Name());

                if (name.empty() == true) {
                    TRACE(Trace::Information, (_T("Registration of a nameless client.")));
                } else {
                    bool allowed = true;

                    if (!_allowedClients.empty()) {
                        std::list<string>::iterator index(_allowedClients.begin());
                        while ((index != _allowedClients.end()) && (name != (*index))) {
                            index++;
                        }
                        if (index == _allowedClients.end()) {
                            TRACE(Trace::Information, (_T("Allowed list not empty and client %s not there."), name.c_str()));
                            allowed = false;
                        }
                    }

                    if (allowed) {
                        std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());

                        while ((index != _clients.end()) && (name != (*index)->Name())) {
                            index++;
                        }

                        if (index != _clients.end()) {
                            TRACE(Trace::Information, (_T("Client already registered %s."), name.c_str()));
                        } else {
                            client->AddRef();
                            _clients.push_back(client);

                            TRACE(Trace::Information, (_T("Added client %s."), name.c_str()));

                            if (_observers.size() > 0) {

                                std::list<Exchange::IComposition::INotification*>::iterator index(_observers.begin());

                                while (index != _observers.end()) {
                                    (*index)->Attached(name, client);
                                    index++;
                                }
                            }
                        }
                    }
                }
            }
        }
        void Remove(const char clientName[])
        {
            const std::string name(clientName);

            if (name.empty() == true) {
                TRACE(Trace::Information, (_T("Unregistering a nameless client.")));
            } else {
                std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());

                while ((index != _clients.end()) && (name != (*index)->Name())) {
                    index++;
                }

                if (index == _clients.end()) {
                    TRACE(Trace::Information, (_T("Client already unregistered %s."), name.c_str()));
                } else {
                    Exchange::IComposition::IClient* entry(*index);

                    _clients.erase(index);

                    TRACE(Trace::Information, (_T("Removed client %s."), name.c_str()));

                    if (_observers.size() > 0) {
                        std::list<Exchange::IComposition::INotification*>::iterator index(_observers.begin());

                        while (index != _observers.end()) {
                            (*index)->Detached(name);
                            index++;
                        }
                    }

                    entry->Release();
                }
            }
        }
        void PlatformReady()
        {
            if (_delay != nullptr)
                Resolution(_displayFormat);

            PluginHost::ISubSystem* subSystems(_service->SubSystems());

            ASSERT(subSystems != nullptr);

            if (subSystems != nullptr) {
                // Set Graphics event. We need to set up a handler for this at a later moment
                subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
                subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                subSystems->Release();

                // Now the platform is ready we should Join it as well, since we
                // will do generic (not client related) stuff as well.
#ifndef NEXUS_SERVER_EXTERNAL
                if ((_nxserver != nullptr) && (Join() != true)) {
                    TRACE(Trace::Information, (_T("Could not Join the started NXServer.")));
                }
#else
                if (Join() != true) {
                    TRACE(Trace::Information, (_T("Could not Join the NXServer.")));
                }
#endif
            }
        }
        inline void HardwareDelay(const uint16_t time, Exchange::IComposition::ScreenResolution format)
        {
            ASSERT(_delay == nullptr);
            if ((time != 0) && (_delay == nullptr)) {
                _delay = new Postpone(*this, time);
            }
            _displayFormat = format;
        }
        inline bool Join()
        {
            if ((_joined == false) && (NxClient_Join(&_joinSettings) == NEXUS_SUCCESS)) {
                _joined = true;
                NxClient_UnregisterAcknowledgeStandby(NxClient_RegisterAcknowledgeStandby());
            }
            return (_joined);
        }

    private:
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        std::list<Exchange::IComposition::INotification*> _observers;
        std::list<Exchange::IComposition::IClient*> _clients;
        std::list<string> _allowedClients;

        bool _joined;
        NxClient_JoinSettings _joinSettings;
        Exchange::IComposition::ScreenResolution _displayFormat;
        Postpone* _delay;

#ifndef NEXUS_SERVER_EXTERNAL
        Sink _sink;
        Broadcom::Platform* _nxserver;
#endif
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
