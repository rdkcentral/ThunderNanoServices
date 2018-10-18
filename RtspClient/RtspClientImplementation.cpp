#include "Module.h"
#include <interfaces/IRtspClient.h>
#include <interfaces/IMemory.h>

namespace WPEFramework {
namespace Plugin {

    class RtspClientImplementation : public Exchange::IRtspClient {
    private:

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , TestNum(0)
            {
                Add(_T("testNum"), &TestNum);
                Add(_T("testStr"), &TestStr);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 TestNum;
            Core::JSON::String TestStr;
        };

    private:
        RtspClientImplementation(const RtspClientImplementation&) = delete;
        RtspClientImplementation& operator=(const RtspClientImplementation&) = delete;

    public:
        RtspClientImplementation()
            : _observers()
            , str("Nothing set")
        {
        }

        virtual ~RtspClientImplementation()
        {
        }

        uint32_t Configure(PluginHost::IShell* service)
        {
            ASSERT(service != nullptr);

            Config config;
            config.FromString(service->ConfigLine());

            uint32_t result = 0;

            return (result);
        }

        uint32_t Setup(const string& assetId, uint32_t position)
        {
            return 0;
        }

        uint32_t Play(int16_t scale, uint32_t position)
        {
            return 0;
        }

        uint32_t Teardown()
        {
            return 0;
        }

        void Set(const string& name, const string& value)
        {
        }

        string Get(const string& value) const
        {
            return (str);
        }

        BEGIN_INTERFACE_MAP(RtspClientImplementation)
        INTERFACE_ENTRY(Exchange::IRtspClient)
        END_INTERFACE_MAP

    private:
        std::list<PluginHost::IStateControl::INotification*> _observers;
        string str;
    };

    SERVICE_REGISTRATION(RtspClientImplementation, 1, 0);

} // namespace Plugin

} // namespace WPEFramework
