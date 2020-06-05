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

#include "HID.h"

namespace WPEFramework {

namespace USB {

/* static */ bool HID::ParseReportMap(const uint16_t length, const uint8_t data[], Collection& rootCollection)
{
    const uint8_t* p = data;
    const uint8_t* pEnd = data + length;

    Report* report = nullptr;
    Collection* collection = &rootCollection;

    uint16_t usagePage = 0;
    uint16_t count = 0;
    uint16_t size = 0;
    uint32_t logicalMin = 0;
    uint32_t logicalMax = 0;
    std::list<uint32_t> usages;

    bool failure = false;

    while ((p < pEnd) && (failure == false)) {

        auto Read = [&](const uint8_t size) -> uint32_t {
            uint32_t val = 0;
            if (size == 1) {
                if (p + 1 < pEnd) {
                    val = p[0];
                }
                p++;
            } else if (size == 2) {
                if (p + 2 < pEnd) {
                    val = (p[0] | (p[1] << 8));
                }
                p += 2;
            } else if (size == 3) {
                if (p + 4 < pEnd) {
                    val = (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
                }
                p += 4;
            } else {
                failure = true;
            }
            return (val);
        };

        auto CollectElement = [&](const Report::Element::category type, const uint32_t flags) {
            if (report == nullptr) {
                // no report created yet, possibly a single report descriptor, create it now
                report = &collection->Add();
            }
            report->Add(type, usages, flags, count, size, logicalMin, logicalMax);
        };

        const uint8_t descriptor = *p++;
        if (descriptor == 0xFC) {
            // extended descriptor, not supported
            failure = true;
            break;
        }

        const uint8_t elem = (descriptor >> 4);
        const uint8_t type = (descriptor >> 2) & 0x3;
        const uint8_t bytes = descriptor & 0x3;

        switch (type) {
        case 0: // main
            switch (elem) {
            case 8: // input
                CollectElement(Report::Element::INPUT, Read(bytes));
                break;
            case 9: // output
                CollectElement(Report::Element::OUTPUT, Read(bytes));
                break;
            case 11: // feature
                CollectElement(Report::Element::FEATURE, Read(bytes));
                break;
            case 10: // collection
                collection = &collection->Child(static_cast<Collection::category>(Read(bytes)),
                                                (usages.empty() == true? 0 : usages.front()));
                break;
            case 12: // end collection
                if (collection->Type() != Collection::ROOT) {
                    collection = &collection->Parent();
                } else {
                    failure = true;
                }
                break;
            default:
                Read(bytes);
                break;
            }
            // clear local items...
            usages.clear();
            logicalMin = 0;
            logicalMax = 0;
            break;
        case 1: // globals, persistent across "main" items
            switch (elem) {
            case 0: // usage page
                usagePage = Read(bytes);
                break;
            case 7: // size
                size = Read(bytes);
                break;
            case 8: // ID
                report = &collection->Add(Read(bytes));
                break;
            case 9: // count
                count = Read(bytes);
                break;
            default:
                Read(bytes);
                break;
            }
            break;
        case 2: // locals, invalidated each "main" item
            switch (elem) {
            case 0: // usage
                if (bytes == 3) {
                    // special case to read both page and usage
                    usages.push_back(Read(bytes));
                } else {
                    usages.push_back(MakeUsage(usagePage, Read(bytes)));
                }
                break;
            case 1: // logical minimum
                logicalMin = Read(bytes);
                break;
            case 2: // logical maximum
                logicalMax = Read(bytes);
                break;
            default:
                Read(bytes);
                break;
            }
        }
    }

    return (!failure && (p == pEnd));
}

} // namespace USB

}

