#ifndef __WHITELISTEDORIGINDOMAINSLIST_H
#define __WHITELISTEDORIGINDOMAINSLIST_H

#include "Module.h"

#include <WPE/WebKit.h>

#include <vector>

namespace WPEFramework {
namespace WebKit {

    class WhiteListedOriginDomainsList {
    private:
        WhiteListedOriginDomainsList (const WhiteListedOriginDomainsList&) = delete;
        WhiteListedOriginDomainsList& operator=  (const WhiteListedOriginDomainsList&) = delete;

    public:
        typedef std::pair<bool,string> Domain;
        typedef std::vector<Domain> Domains;
        typedef std::map<string, Domains> WhiteMap;

    public:
        static std::unique_ptr<WhiteListedOriginDomainsList> RequestFromWPEFramework(WKBundleRef bundle);
        ~WhiteListedOriginDomainsList() {
        }

    public:
        void AddWhiteListToWebKit(WKBundleRef bundle);

        // Name of message posted between WPEFramework and WPEWebProcess.
        static inline string GetMessageName()
        {
            return Core::ClassNameOnly(typeid(WhiteListedOriginDomainsList).name()).Data();
        }

    private:
        WhiteListedOriginDomainsList() {
        }

        WhiteMap _whiteMap;
    };
}
}

#endif // __WHITELISTEDORIGINDOMAINSLIST_H
