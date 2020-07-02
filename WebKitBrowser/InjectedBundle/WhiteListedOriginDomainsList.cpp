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
 
#include "WhiteListedOriginDomainsList.h"

#ifndef WEBKIT_GLIB_API
#include "Utils.h"
#include "Tags.h"
#endif

using std::unique_ptr;
using std::vector;

// For now we report errors to stderr.
// TODO: does the injected bundle need a more formal way of dealing with errors?
#include <iostream>
using std::cerr;
using std::endl;

namespace WPEFramework {
namespace WebKit {

    // Parses JSON containing white listed CORS origin-domain pairs.
    static void ParseWhiteList(const string& jsonString, WhiteListedOriginDomainsList::WhiteMap& info)
    {
        // Origin/Domain pair stored in JSON string.
        class JSONEntry : public Core::JSON::Container {
        private:
            JSONEntry& operator=(const JSONEntry&) = delete;

        public:
            JSONEntry()
                : Core::JSON::Container()
                , Origin()
                , Domain()
                , SubDomain(true)
            {
                Add(_T("origin"), &Origin);
                Add(_T("domain"), &Domain);
                Add(_T("subdomain"), &SubDomain);
            }
            JSONEntry(const JSONEntry& rhs)
                : Core::JSON::Container()
                , Origin(rhs.Origin)
                , Domain(rhs.Domain)
                , SubDomain(rhs.SubDomain)
            {
                Add(_T("origin"), &Origin);
                Add(_T("domain"), &Domain);
                Add(_T("subdomain"), &SubDomain);
            }

        public:
            Core::JSON::String Origin;
            Core::JSON::ArrayType<Core::JSON::String> Domain;
            Core::JSON::Boolean SubDomain;
        };

        Core::JSON::ArrayType<JSONEntry> entries;
        entries.FromString(jsonString);
        Core::JSON::ArrayType<JSONEntry>::Iterator originIndex(entries.Elements());

        while (originIndex.Next() == true) {

            if ((originIndex.Current().Origin.IsSet() == true) && (originIndex.Current().Domain.IsSet() == true)) {

                WhiteListedOriginDomainsList::Domains& domains(info[originIndex.Current().Origin.Value()]);

                Core::JSON::ArrayType<Core::JSON::String>::Iterator domainIndex(originIndex.Current().Domain.Elements());
                bool subDomain(originIndex.Current().SubDomain.Value());

                while (domainIndex.Next()) {
                    domains.emplace_back(subDomain, domainIndex.Current().Value());
                }
            }
        }
    }

    // Gets white list from WPEFramework via synchronous message.
    /* static */unique_ptr<WhiteListedOriginDomainsList> WhiteListedOriginDomainsList::RequestFromWPEFramework(const char* whitelist)
    {
#ifdef WEBKIT_GLIB_API
        unique_ptr<WhiteListedOriginDomainsList> whiteList(new WhiteListedOriginDomainsList());
        ParseWhiteList(whitelist, whiteList->_whiteMap);
#else
        string messageName(string(Tags::Config) + "Whitelist");
        std::string utf8MessageName = Core::ToString(messageName.c_str());

        WKStringRef jsMessageName = WKStringCreateWithUTF8CString(utf8MessageName.c_str());
        WKMutableArrayRef messageBody = WKMutableArrayCreate();
        WKTypeRef returnData;

        WKBundlePostSynchronousMessage(WebKit::Utils::GetBundle(), jsMessageName, messageBody, &returnData);

        WKStringRef returnedString = static_cast<WKStringRef>(returnData);

        string jsonString = WebKit::Utils::WKStringToString(returnedString);

        unique_ptr<WhiteListedOriginDomainsList> whiteList(new WhiteListedOriginDomainsList());
        ParseWhiteList(jsonString, whiteList->_whiteMap);

        WKRelease(returnData);
        WKRelease(messageBody);
        WKRelease(jsMessageName);

#endif
        return whiteList;
    }

    // Adds stored entries to WebKit.
    void WhiteListedOriginDomainsList::AddWhiteListToWebKit(WKBundleRef bundle)
    {
        WhiteMap::const_iterator index(_whiteMap.begin());

        while (index != _whiteMap.end()) {

#ifdef WEBKIT_GLIB_API
            WebKitSecurityOrigin* origin = webkit_security_origin_new_for_uri(index->first.c_str());

            for (const Domain& domainIndex : index->second) {

                WebKitSecurityOrigin* domain = webkit_security_origin_new_for_uri(domainIndex.second.c_str());
                webkit_web_extension_add_origin_access_whitelist_entry(bundle,
                        origin, webkit_security_origin_get_protocol(domain),
                        webkit_security_origin_get_host(domain), domainIndex.first);
                webkit_security_origin_unref(domain);
            }
            webkit_security_origin_unref(origin);
#else
            WKStringRef wkOrigin = WKStringCreateWithUTF8CString(index->first.c_str());

            for (const Domain& domain : index->second) {

                std::string utf8Domain = Core::ToString(domain.second.c_str());
                WKURLRef url = WKURLCreateWithUTF8CString(utf8Domain.c_str());
                WKStringRef protocol = WKURLCopyScheme(url);
                WKStringRef host = WKURLCopyHostName(url);

                WKRelease(url);

                WKBundleAddOriginAccessWhitelistEntry(bundle, wkOrigin, protocol, host, domain.first);

                WKRelease(host);
                WKRelease(protocol);

                cerr << "Added origin->domain pair to WebKit white list: " << index->first << " -> " << domain.second << endl;
            }

            WKRelease(wkOrigin);
#endif
            index++;
        }
    }
}
}
