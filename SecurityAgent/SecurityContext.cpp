// C++ program to demonstrate working of regex_search()
#include <iostream>
#include <string.h>
#include <regex>

int testApp()
{
    // Target sequence
    std::string s = "I am looking for GeeksForGeeks "
                    "articles";

    // An object of regex for pattern to be searched
    std::regex r("Geek[a-zA-Z]+");

    // flag type for determining the matching behavior
    // here it is for matches on 'string' objects
    std::smatch m;

    // regex_search() for searching the regex pattern
    // 'r' in the string 's'. 'm' is flag for determining
    // matching behavior.
    std::regex_search(s, m, r);

    // for each loop
    for (auto x : m)
        std::cout << x << " ";

    return 0;
}

#include "SecurityContext.h"

namespace WPEFramework {
namespace Plugin {

    SecurityContext::SecurityContext(const AccessControlList* acl, const uint16_t length, const uint8_t payload[])
        : _accessControlList(nullptr)
    {
        _context.FromString(string(reinterpret_cast<const TCHAR*>(payload), length));

        if ( (_context.URL.IsSet() == true) && (acl != nullptr) ) {
            _accessControlList = acl->FilterMapFromURL(_context.URL.Value());
        }
    }

    /* virtual */ SecurityContext::~SecurityContext()
    {
    }

    //! Allow a websocket upgrade to be checked if it is allowed to be opened.
    bool SecurityContext::Allowed(const string& path) const /* override */
    {
        return (true);
    }

    //! Allow a request to be checked before it is offered for processing.
    bool SecurityContext::Allowed(const Web::Request& request) const /* override */ 
    {
        bool allowed = (_accessControlList != nullptr);

		if (allowed == true) {
			
		}

        return (allowed);
    }

    //! Allow a JSONRPC message to be checked before it is offered for processing.
    bool SecurityContext::Allowed(const Core::JSONRPC::Message& message) const /* override */ 
    {
        return ((_accessControlList != nullptr) && (_accessControlList->Allowed(message.Callsign())));
    }
}
}
