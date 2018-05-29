#ifndef LOCATIONSYNC_LOCATIONSYNC_H
#define LOCATIONSYNC_LOCATIONSYNC_H

#include "Module.h"
#include "LocationService.h"

namespace WPEFramework {
namespace Plugin {

    class LocationSync : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        class Data : public Core::JSON::Container {
        public:
            Data(Data const& other) = delete;
            Data& operator=(Data const& other) = delete;

            Data()
                : Core::JSON::Container()
                , PublicIp()
                , TimeZone()
                , Region()
                , Country()
                , City()
            {
                Add(_T("ip"), &PublicIp);
                Add(_T("timezone"), &TimeZone);
                Add(_T("region"), &Region);
                Add(_T("country"), &Region);
                Add(_T("city"), &Region);
            }

            virtual ~Data()
            {
            }

        public:
            Core::JSON::String PublicIp;
            Core::JSON::String TimeZone;
            Core::JSON::String Region;
            Core::JSON::String Country;
            Core::JSON::String City;
        };

    private:
        class Notification : public Core::IDispatch {
        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
            explicit Notification(LocationSync* parent)
                : _parent(*parent)
                , _source()
                , _interval()
                , _retries()
                , _locator(Core::Service<LocationService>::Create<LocationService>(this))
            {
                ASSERT(parent != nullptr);
            }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif
            ~Notification()
            {
				_locator->Release();
            }

        public:
            inline void Initialize(PluginHost::IShell* service, const string& source, const uint16_t interval, const uint8_t retries)
            {
                _source = source;
                _interval = interval;
                _retries = retries;

		Probe();
            }
            inline void Deinitialize()
            {
            }
            uint32_t Probe(const string& remoteNode, const uint32_t retries, const uint32_t retryTimeSpan)
            {
                _source = remoteNode;
                _interval = retryTimeSpan;
                _retries = retries;

                return (Probe());
            }

            inline PluginHost::ISubSystem::ILocation* Location () {
                return (_locator);
            }
            inline PluginHost::ISubSystem::IInternet* Network() {
                return (_locator);
            }

        private:
            inline uint32_t Probe() {

                ASSERT (_locator != nullptr);

                return (_locator != nullptr ? _locator->Probe(_source, _retries, _interval) : Core::ERROR_UNAVAILABLE);
            }

            virtual void Dispatch()
            {
                _parent.SyncedLocation();
            }

        private:
            LocationSync& _parent;
            string _source;
            uint16_t _interval;
            uint8_t _retries;
            LocationService* _locator;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Interval(30)
                , Retries(8)
                , Source()
            {
                Add(_T("interval"), &Interval);
                Add(_T("retries"), &Retries);
                Add(_T("source"), &Source);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Interval;
            Core::JSON::DecUInt8 Retries;
            Core::JSON::String Source;
        };

    private:
        LocationSync(const LocationSync&) = delete;
        LocationSync& operator=(const LocationSync&) = delete;

    public:
        LocationSync();
        virtual ~LocationSync();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(LocationSync)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        void SyncedLocation();

    private:
        uint16_t _skipURL;
        string _source;
        Core::Sink<Notification> _sink;
        PluginHost::IShell* _service;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // LOCATIONSYNC_LOCATIONSYNC_H
