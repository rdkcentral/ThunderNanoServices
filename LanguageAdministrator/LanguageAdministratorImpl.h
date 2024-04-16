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

#pragma once

#include "Module.h"
#include <interfaces/ILanguageTag.h>

namespace Thunder {
namespace Plugin {

    class LanguageAdministratorImpl : public Exchange::ILanguageTag {
    public:
        LanguageAdministratorImpl(const LanguageAdministratorImpl&) = delete;
        LanguageAdministratorImpl& operator=(const LanguageAdministratorImpl&) = delete;

        LanguageAdministratorImpl();
        ~LanguageAdministratorImpl() override;

        BEGIN_INTERFACE_MAP(LanguageAdministratorImpl)
            INTERFACE_ENTRY(Exchange::ILanguageTag)
        END_INTERFACE_MAP

        //   ILanguageTag methods
        void Register(Exchange::ILanguageTag::INotification* observer) override;
        void Unregister(const Exchange::ILanguageTag::INotification* observer) override;

        uint32_t Language(string& language /* @out */) const override;
        uint32_t Language(const string& language) override;

    private:
        string _language;
        Core::CriticalSection _adminLock;
        std::vector<Exchange::ILanguageTag::INotification*> _notifications;

        void NotifyLanguageChanged(const string& language);
    };

}  // namespace Plugin
}  // namespace Thunder
