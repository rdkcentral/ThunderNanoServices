/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef __WHITELISTEDORIGINDOMAINSLIST_H
#define __WHITELISTEDORIGINDOMAINSLIST_H

#include "Module.h"

#ifdef WEBKIT_GLIB_API
#include <wpe/webkit-web-extension.h>
typedef WebKitWebExtension* WKBundleRef;
#else
#include <WPE/WebKit.h>
#endif

#include <vector>

namespace WPEFramework {
namespace WebKit {

    class WhiteListedOriginDomainsList {
    private:
        WhiteListedOriginDomainsList(const WhiteListedOriginDomainsList&) = delete;
        WhiteListedOriginDomainsList& operator=(const WhiteListedOriginDomainsList&) = delete;

    public:
        typedef std::pair<bool, string> Domain;
        typedef std::vector<Domain> Domains;
        typedef std::map<string, Domains> WhiteMap;

    public:
        static std::unique_ptr<WhiteListedOriginDomainsList> RequestFromWPEFramework(const char* whitelist = nullptr);
        ~WhiteListedOriginDomainsList()
        {
        }

    public:
        void AddWhiteListToWebKit(WKBundleRef bundle);

    private:
        WhiteListedOriginDomainsList()
        {
        }

        WhiteMap _whiteMap;
    };
}
}

#endif // __WHITELISTEDORIGINDOMAINSLIST_H
