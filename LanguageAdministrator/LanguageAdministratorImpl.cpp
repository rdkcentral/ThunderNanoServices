/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "LanguageAdministratorImpl.h"
#include <unicode/uloc.h>

namespace Thunder {
namespace Plugin {

    SERVICE_REGISTRATION(LanguageAdministratorImpl, 1, 0)

    LanguageAdministratorImpl::LanguageAdministratorImpl()
        : _adminLock()

    {
    }

    LanguageAdministratorImpl::~LanguageAdministratorImpl() = default;

    void LanguageAdministratorImpl::Register(Exchange::ILanguageTag::INotification* notification)
    {
        ASSERT(notification);

        _adminLock.Lock();
        auto item = find(_notifications.begin(), _notifications.end(), notification);
        ASSERT(item == _notifications.end());
        if (item == _notifications.end()) {
            notification->AddRef();
            _notifications.push_back(notification);
        }
        _adminLock.Unlock();
    }

    void LanguageAdministratorImpl::Unregister(const Exchange::ILanguageTag::INotification* notification)
    {
        ASSERT(notification);

        _adminLock.Lock();
        auto item = find(_notifications.begin(), _notifications.end(), notification);
        ASSERT(item != _notifications.end());
        if (item != _notifications.end()) {
            _notifications.erase(item);
            (*item)->Release();
        }
        _adminLock.Unlock();
    }

    uint32_t LanguageAdministratorImpl::Language(const string& language) {
        //set the language code
        bool bInitialize = false;
        _adminLock.Lock();

        if (_language.empty()){
            //Setting default value. Notification is not required.
            bInitialize = true;
        }

        if ( (language != _language) && ([&](const string& lang){
                    const char* langCode = uloc_getISO3Language(lang.c_str());
                    return (langCode[0] == '\0') ? false : true;}(language)) )
        {
            _language = language;
            if (!bInitialize)
                NotifyLanguageChanged(language);
        }

        _adminLock.Unlock();
        return Core::ERROR_NONE;

    }

    uint32_t LanguageAdministratorImpl::Language(string& language /* @out */) const {
        language = _language;
        TRACE(Trace::Information, (_T("Get Language: %d"), language));
        return Core::ERROR_NONE;
    }

    void LanguageAdministratorImpl::NotifyLanguageChanged(const string& language)
    {
        _adminLock.Lock();
        for (auto* notification : _notifications) {
            notification->LanguageChanged(language);
        }
        _adminLock.Unlock();
    }
}  // namespace Plugin
}  // namespace Thunder
