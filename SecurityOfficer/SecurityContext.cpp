// C++ program to demonstrate working of regex_search()
#include <iostream>
#include <regex>
#include <string.h>

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

	SecurityContext::SecurityContext(const uint16_t length, const uint8_t payload[]) {

	}

	/* virtual */ SecurityContext::~SecurityContext()
    {
	}
    
	//! Allow a request to be checked before it is offered for processing.
	/* virtual */ bool SecurityContext::Allowed(const Web::Request& request) const {
        return (true);
	}

    //! Allow a JSONRPC message to be checked before it is offered for processing.
	/* virtual */ bool SecurityContext::Allowed(const Core::JSONRPC::Message& message) const {
        return (true);
    }

    }
}
