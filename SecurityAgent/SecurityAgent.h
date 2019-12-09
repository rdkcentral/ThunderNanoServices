#pragma once

#include "Module.h"
#include <interfaces/json/JsonData_SecurityAgent.h>
#include "AccessControlList.h"


namespace WPEFramework {
namespace Plugin {

    class SecurityAgent : public PluginHost::IAuthenticate,
                            public PluginHost::IPlugin,
                            public PluginHost::JSONRPC,
                            public PluginHost::IWeb {
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
        SecurityAgent(const SecurityAgent&) = delete;
        SecurityAgent& operator=(const SecurityAgent&) = delete;

        SecurityAgent();
        virtual ~SecurityAgent();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(SecurityAgent)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
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

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        //! Whenever a request is received, it might carry some additional data in the body. This method allows
        //! the plugin to attach a deserializable data object (ref counted) to be loaded with any potential found
        //! in the body of the request.
        //! @}
        virtual void Inbound(Web::Request& request);

        //! @{
        //! ==================================== CALLED ON THREADPOOL THREAD ======================================
        //! If everything is received correctly, the request is passed to us, on a thread from the thread pool, to
        //! do our thing and to return the result in the response object. Here the actual specific module work,
        //! based on a a request is handled.
        //! @}
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

    private:
        //   JsonRPC methods
        // -------------------------------------------------------------------------------------------------------
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_createtoken(const JsonData::SecurityAgent::CreatetokenParamsData& params, JsonData::SecurityAgent::CreatetokenResultInfo& response);
        uint32_t endpoint_validate(const JsonData::SecurityAgent::CreatetokenResultInfo& params, JsonData::SecurityAgent::ValidateResultData& response);


    private:
        uint8_t _secretKey[Crypto::SHA256::Length];
        AccessControlList _acl;
        uint8_t _skipURL;
    };

} // namespace Plugin
} // namespace WPEFramework
