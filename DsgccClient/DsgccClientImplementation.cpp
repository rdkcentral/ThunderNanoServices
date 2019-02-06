#include "Module.h"
#include <interfaces/IDsgccClient.h>
#include <interfaces/IMemory.h>

namespace WPEFramework {
namespace Plugin {

    class DsgccClientImplementation : public Exchange::IDsgccClient {
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
        DsgccClientImplementation(const DsgccClientImplementation&) = delete;
        DsgccClientImplementation& operator=(const DsgccClientImplementation&) = delete;

    public:
        DsgccClientImplementation()
            : _observers()
            , _worker()
            , str("Nothing set")
        {
        }

        virtual ~DsgccClientImplementation()
        {
        }

        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            ASSERT(service != nullptr);

            Config config;
            config.FromString(service->ConfigLine());

            uint32_t result = 0;

            return (result);
        }

        virtual void DsgccClientSet(const string& str)
        {
            this->str = str;
        }
        virtual string DsgccClientGet() const
        {
            return (str);
        }

        BEGIN_INTERFACE_MAP(DsgccClientImplementation)
        INTERFACE_ENTRY(Exchange::IDsgccClient)
        END_INTERFACE_MAP

    private:
        std::list<PluginHost::IStateControl::INotification*> _observers;
        string str;
    };

    SERVICE_REGISTRATION(DsgccClientImplementation, 1, 0);

} // namespace Plugin

} // namespace WPEFramework
