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

#include "SubsystemControl.h"

namespace Thunder {

namespace Plugin {

    namespace {

        static Metadata<SubsystemControl> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    struct JSecurity : public PluginHost::ISubSystem::ISecurity, public Core::JSON::Container {
    public:
        JSecurity(const JSecurity&) = delete;
        JSecurity& operator= (const JSecurity&) = delete;

        JSecurity()
            : Core::JSON::Container()
            , _callsign() {
            Add(_T("callsign"), &_callsign);
        }
        ~JSecurity() override = default;

    public:
        string Callsign() const override {
            return (_callsign.Value());
        }

    private:
        BEGIN_INTERFACE_MAP(JSecurity)
            INTERFACE_ENTRY(PluginHost::ISubSystem::ISecurity)
        END_INTERFACE_MAP

    private:
        Core::JSON::String _callsign;
    };

    struct JInternet : public PluginHost::ISubSystem::IInternet, public Core::JSON::Container {
    public:
        JInternet(const JInternet&) = delete;
        JInternet& operator= (const JInternet&) = delete;

        JInternet()
            : Core::JSON::Container()
            , _IP()
            , _type(UNKNOWN) {
            Add(_T("ip"), &_IP);
            Add(_T("type"), &_type);
        }
        ~JInternet() override = default;

    public:
        string PublicIPAddress() const override {
            return (_IP.Value());
        }
        network_type NetworkType() const override {
            return (_type.Value());
        }

    private:
        BEGIN_INTERFACE_MAP(JInternet)
            INTERFACE_ENTRY(PluginHost::ISubSystem::IInternet)
        END_INTERFACE_MAP

    private:
        Core::JSON::String _IP;
        Core::JSON::EnumType<network_type> _type;
    };

    struct JLocation : public PluginHost::ISubSystem::ILocation, public Core::JSON::Container {
    public:
        JLocation(const JLocation&) = delete;
        JLocation& operator= (const JLocation&) = delete;

        JLocation()
            : Core::JSON::Container()
            , _timeZone()
            , _country()
            , _region()
            , _city()
            , _latitude(ILocation::Latitude())
            , _longitude(ILocation::Longitude()) {
            Add(_T("timezone"), &_timeZone);
            Add(_T("country"), &_country);
            Add(_T("region"), &_region);
            Add(_T("city"), &_city);
            Add(_T("latitude"), &_latitude);
            Add(_T("longitude"), &_longitude);
        }
        ~JLocation() override = default;

    public:
        string TimeZone() const override {
            return (_timeZone.Value());
        }
        string Country() const override {
            return (_country.Value());
        }
        string Region() const override {
            return (_region.Value());
        }
        string City() const override {
            return (_city.Value());
        }
        int32_t Latitude() const override {
            return (_latitude.Value());
        }
        int32_t Longitude() const override {
            return (_longitude.Value());
        }

    private:
        BEGIN_INTERFACE_MAP(JLocation)
            INTERFACE_ENTRY(PluginHost::ISubSystem::ILocation)
        END_INTERFACE_MAP

    private:
        Core::JSON::String _timeZone;
        Core::JSON::String _country;
        Core::JSON::String _region;
        Core::JSON::String _city;
        Core::JSON::DecSInt32 _latitude;
        Core::JSON::DecSInt32 _longitude;
    };

    struct JIdentifier : public PluginHost::ISubSystem::IIdentifier, public Core::JSON::Container {
    public:
        JIdentifier(const JIdentifier&) = delete;
        JIdentifier& operator= (const JIdentifier&) = delete;

        JIdentifier()
            : Core::JSON::Container()
            , _identifier()
            , _architecture()
            , _chipset()
            , _firmwareVersion() {
            // The identiifer is a Base64 encoded string...
            Add(_T("identifier"), &_identifier);
            Add(_T("architecture"), &_architecture);
            Add(_T("chipset"), &_chipset);
            Add(_T("firmware"), &_firmwareVersion);
        }
        ~JIdentifier() override = default;

    public:
        uint8_t Identifier(const uint8_t length, uint8_t buffer[] /*@maxlength:length @out*/) const override {
            uint16_t maxLength = length;
            Core::FromString(_identifier.Value(), buffer, maxLength);
            return (static_cast<uint8_t>(maxLength & 0xFF));
        }
        string Architecture() const override {
            return (_architecture.Value());
        }
        string Chipset() const override {
            return (_chipset.Value());
        }
        string FirmwareVersion() const override {
            return (_firmwareVersion.Value());
        }

    private:
        BEGIN_INTERFACE_MAP(JIdentifier)
            INTERFACE_ENTRY(PluginHost::ISubSystem::IIdentifier)
        END_INTERFACE_MAP

    private:
        Core::JSON::String _identifier;
        Core::JSON::String _architecture;
        Core::JSON::String _chipset;
        Core::JSON::String _firmwareVersion;
    };

    struct JTime : public PluginHost::ISubSystem::ITime, public Core::JSON::Container {
    public:
        JTime(const JTime&) = delete;
        JTime& operator= (const JTime&) = delete;

        JTime()
            : Core::JSON::Container()
            , _time() {
            Add(_T("time"), &_time);
        }
        ~JTime() override = default;

    public:
        uint64_t TimeSync() const override {
            Core::Time value; value.FromISO8601(_time.Value().c_str());
            return (value.Ticks());
        }

    private:
        BEGIN_INTERFACE_MAP(JTime)
            INTERFACE_ENTRY(ITime)
        END_INTERFACE_MAP

    private:
        Core::JSON::String _time;
    };

    struct JProvisioning : public PluginHost::ISubSystem::IProvisioning, public Core::JSON::Container {
    private:
        using List = Core::JSON::ArrayType<Core::JSON::String>;

    public:
        JProvisioning(const JProvisioning&) = delete;
        JProvisioning& operator= (const JProvisioning&) = delete;

        JProvisioning()
            : Core::JSON::Container()
            , _storage()
            , _systems()
            , _index(~0)
            , _current() {
            Add(_T("storage"), &_storage);
            Add(_T("systems"), &_systems);
        }
        ~JProvisioning() override = default;

    public:
        string Storage() const override {
            return (_storage.Value());
        }
        bool Next(string& info /* @out */) override {
            uint32_t size = _systems.Elements().Count();
            if (_index != size) {
                _index++;
                if (_index != size) {
                    _current = Index(_index);
                    info = _current;
                }
            }
            return (_index != size);
        }
        bool Previous(string& info /* @out */) override {
            if (_index == 0) {
                _index = ~0;
            }
            else {
                _index--;
                _current = Index(_index);
                info = _current;
            }
            return (_index != static_cast<uint32_t>(~0));
        }
        void Reset(const uint32_t position) override {
            _index = position;
            if (position == static_cast<uint32_t>(~0)) {
                _index = ~0;
                _current.clear();
            }
            else if (position >= _systems.Elements().Count()) {
                _index = _systems.Elements().Count();
                _current.clear();
            }
            else {
                _index = position;
                _current = Index(_index);
            }
        }
        bool IsValid() const override {
            return ((_index != static_cast<uint32_t>(~0)) && (_index < _systems.Elements().Count()));
        }
        uint32_t Count() const override {
            return (_systems.Elements().Count());
        }
        string Current() const override {
            return (_current);
        }

    private:
        BEGIN_INTERFACE_MAP(JProvisioning)
            INTERFACE_ENTRY(PluginHost::ISubSystem::IProvisioning)
        END_INTERFACE_MAP

        string Index(const uint32_t index) const {
            List::ConstIterator loop(_systems.Elements());
            return (loop.Reset(index) == true ? loop.Current().Value() : string());
        }

    private:
        Core::JSON::String _storage;
        List _systems;
        uint32_t _index;
        string _current;
    };

    struct JDecryption : public PluginHost::ISubSystem::IDecryption, public Core::JSON::Container {
    private:
        using List = Core::JSON::ArrayType<Core::JSON::String>;

    public:
        JDecryption(const JDecryption&) = delete;
        JDecryption& operator= (const JDecryption&) = delete;

        JDecryption()
            : Core::JSON::Container()
            , _systems()
            , _index(~0)
            , _current() {
            Add(_T("systems"), &_systems);
        }
        ~JDecryption() override = default;

    public:
        bool Next(string& info /* @out */) override {
            uint32_t size = _systems.Elements().Count();
            if (_index != size) {
                _index++;
                if (_index != size) {
                    _current = Index(_index);
                    info = _current;
                }
            }
            return (_index != size);
        }
        bool Previous(string& info /* @out */) override {
            if (_index == 0) {
                _index = ~0;
            }
            else {
                _index--;
                _current = Index(_index);
                info = _current;
            }
            return (_index != static_cast<uint32_t>(~0));
        }
        void Reset(const uint32_t position) override {
            _index = position;
            if (position == static_cast<uint32_t>(~0)) {
                _index = ~0;
                _current.clear();
            }
            else if (position >= _systems.Elements().Count()) {
                _index = _systems.Elements().Count();
                _current.clear();
            }
            else {
                _index = position;
                _current = Index(_index);
            }
        }
        bool IsValid() const override {
            return ((_index != static_cast<uint32_t>(~0)) && (_index < _systems.Elements().Count()));
        }
        uint32_t Count() const override {
            return (_systems.Elements().Count());
        }
        string Current() const override {
            return (_current);
        }

    private:
        BEGIN_INTERFACE_MAP(JDecryption)
            INTERFACE_ENTRY(PluginHost::ISubSystem::IDecryption)
        END_INTERFACE_MAP

        string Index(const uint32_t index) const {
            List::ConstIterator loop(_systems.Elements());
            return (loop.Reset(index) == true ? loop.Current().Value() : string());
        }

    private:
        List _systems;
        uint32_t _index;
        string _current;
    };

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    SubsystemControl::SubsystemControl()
        : _subsystemFactory()
        , _service(nullptr)
        , _notification(*this) {
        _subsystemFactory.Announce<JSecurity>    (JsonData::SubsystemControl::SubsystemType::SECURITY);
        _subsystemFactory.Announce<JInternet>    (JsonData::SubsystemControl::SubsystemType::INTERNET);
        _subsystemFactory.Announce<JLocation>    (JsonData::SubsystemControl::SubsystemType::LOCATION);
        _subsystemFactory.Announce<JIdentifier>  (JsonData::SubsystemControl::SubsystemType::IDENTIFIER);
        _subsystemFactory.Announce<JTime>        (JsonData::SubsystemControl::SubsystemType::TIME);
        _subsystemFactory.Announce<JProvisioning>(JsonData::SubsystemControl::SubsystemType::PROVISIONING);
        _subsystemFactory.Announce<JDecryption>  (JsonData::SubsystemControl::SubsystemType::DECRYPTION);
        _subsystemFactory.Announce(JsonData::SubsystemControl::SubsystemType::BLUETOOTH, PluginHost::ISubSystem::subsystem::BLUETOOTH);
        _subsystemFactory.Announce(JsonData::SubsystemControl::SubsystemType::GRAPHICS,  PluginHost::ISubSystem::subsystem::GRAPHICS);
        _subsystemFactory.Announce(JsonData::SubsystemControl::SubsystemType::NETWORK,   PluginHost::ISubSystem::subsystem::NETWORK);
        _subsystemFactory.Announce(JsonData::SubsystemControl::SubsystemType::PLATFORM,  PluginHost::ISubSystem::subsystem::PLATFORM);
        _subsystemFactory.Announce(JsonData::SubsystemControl::SubsystemType::STREAMING, PluginHost::ISubSystem::subsystem::STREAMING);
        _subsystemFactory.Announce(JsonData::SubsystemControl::SubsystemType::WEBSOURCE, PluginHost::ISubSystem::subsystem::WEBSOURCE);
        _subsystemFactory.Announce(JsonData::SubsystemControl::SubsystemType::INSTALLATION, PluginHost::ISubSystem::subsystem::INSTALLATION);
    }
POP_WARNING()

    const string SubsystemControl::Initialize(PluginHost::IShell* service) /* override */
    {
        ASSERT(_service == nullptr);

        _service = service->SubSystems();

        ASSERT(_service != nullptr);

        if (_service != nullptr) {
            _service->Register(&_notification);
        }

        return (_service == nullptr ? _T("SubsystemControl could not retrieve the ISubSystem interface.") : EMPTY_STRING);
    }

    void SubsystemControl::Deinitialize(PluginHost::IShell* /* service */)  /* override */
    {
        if (_service != nullptr) {

            _service->Unregister(&_notification);
            _service->Release();
            _service = nullptr;
        }
    }

    string SubsystemControl::Information() const /* override */
    {
        return string();
    }

} // namespace Plugin
} // namespace Thunder
