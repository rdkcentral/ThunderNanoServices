#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

class SecurityContext : public PluginHost::ISecurity {
public:
    SecurityContext() = delete;
    SecurityContext(const SecurityContext&) = delete;
    SecurityContext& operator= (const SecurityContext&) = delete;

	SecurityContext(const uint16_t length, const uint8_t payload[]);
    virtual ~SecurityContext();

    //! Allow a request to be checked before it is offered for processing.
    virtual bool Allowed(const Web::Request& request) const;

    //! Allow a JSONRPC message to be checked before it is offered for processing.
    virtual bool Allowed(const Core::JSONRPC::Message& message) const;

private:
    // Build QueryInterface implementation, specifying all possible interfaces to be returned.
    BEGIN_INTERFACE_MAP(SecurityOfficer)
		INTERFACE_ENTRY(PluginHost::ISecurity)
    END_INTERFACE_MAP
};

} } 
