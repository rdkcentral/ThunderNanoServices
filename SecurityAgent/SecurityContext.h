#pragma once

#include "Module.h"
#include "AccessControlList.h"

namespace WPEFramework {
namespace Plugin {

    class SecurityContext : public PluginHost::ISecurity {
    private:
        class Payload : public Core::JSON::Container {
        public:
            Payload(const Payload&) = delete;
            Payload& operator=(const Payload&) = delete;

            Payload()
                : Core::JSON::Container()
                , URL()
                , User()
                , Hash()
            {
                Add(_T("url"), &URL);
                Add(_T("user"), &User);
                Add(_T("hash"), &Hash);
            }
            ~Payload()
            {
            }

        public:
            Core::JSON::String URL;
            Core::JSON::String User;
            Core::JSON::String Hash;
        };

    public:
        SecurityContext() = delete;
        SecurityContext(const SecurityContext&) = delete;
        SecurityContext& operator=(const SecurityContext&) = delete;

        SecurityContext(const AccessControlList* acl, const uint16_t length, const uint8_t payload[]);
        virtual ~SecurityContext();

        //! Allow a websocket upgrade to be checked if it is allowed to be opened.
        bool Allowed(const string& path) const override;

        //! Allow a request to be checked before it is offered for processing.
        bool Allowed(const Web::Request& request) const override;

        //! Allow a JSONRPC message to be checked before it is offered for processing.
        bool Allowed(const Core::JSONRPC::Message& message) const override;

    private:
        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(SecurityOfficer)
        INTERFACE_ENTRY(PluginHost::ISecurity)
        END_INTERFACE_MAP

    private:
        Payload _context;
        const AccessControlList::Filter* _accessControlList;
    };
}
}
