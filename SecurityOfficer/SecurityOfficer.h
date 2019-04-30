#pragma once

#include "Module.h"
#include "AccessControlList.h"

namespace WPEFramework {
namespace Plugin {

    class SecurityOfficer : public PluginHost::IAuthenticate,
                            public PluginHost::IPlugin,
                            public PluginHost::JSONRPC {
    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , ACL(_T("acl.json"))
            {
                Add(_T("acl"), &ACL);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String ACL;
        };

    public:
        SecurityOfficer(const SecurityOfficer&) = delete;
        SecurityOfficer& operator=(const SecurityOfficer&) = delete;

        SecurityOfficer();
        virtual ~SecurityOfficer();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(SecurityOfficer)
			INTERFACE_ENTRY(PluginHost::IPlugin)
			INTERFACE_ENTRY(PluginHost::IAuthenticate)
			INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IAuthenticate methods
        // -------------------------------------------------------------------------------------------------------
        virtual uint32_t CreateToken(const uint16_t length, const uint8_t buffer[], string& token);
        virtual PluginHost::ISecurity* Officer(const string& token);

    private:
        uint8_t _secretKey[Crypto::SHA256::Length];
        AccessControlList _acl;
    };

} // namespace Plugin
} // namespace WPEFramework
