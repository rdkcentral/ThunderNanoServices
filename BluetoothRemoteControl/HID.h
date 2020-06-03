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

#include "Module.h"

namespace WPEFramework {

namespace USB {

    class HID {
    public:
        enum class usagepage : uint16_t {
            UNDEFINED               = 0x00,
            GENERIC_DESKTOP         = 0x01,
            SIMULATION              = 0x02,
            VIRTUAL_REALITY         = 0x03,
            SPORT                   = 0x04,
            GAME                    = 0x05,
            GENERIC_DEVICE          = 0x06,
            KEYBOARD                = 0x07,
            LED                     = 0x08,
            BUTTON                  = 0x09,
            ORDINAL                 = 0x0A,
            TELEPHONY               = 0x0B,
            CONSUMER                = 0x0C,
            DIGITIZER               = 0x0D,
            PHYSICAL_DEVICE         = 0x0F,
            ALPHANUMERIC_DISPLAY    = 0x14,
            MEDICAL_INSTRUMENT      = 0x40,
            MONITOR_DEVICE          = 0x80,
            POWER_DEVICE            = 0x84,
            BATTERY_SYSTEM          = 0x85,
            BARCODE_SCANNER         = 0x8C,
            SCALE                   = 0x8D,
            CAMERA_CONTROL          = 0x90,
            ARCADE                  = 0x91,
            VENDOR_SPECIFIC         = 0xFF00
        };

        enum class desktopusage : uint16_t {
            POINTER                 = 0x01,
            MOUSE,
            JOYSTICK,
            GAMEPAD,
            KEYBOARD,
            KEYPAD,
            MULTI_AXIS,
            TABLET
        };

        enum class consumerusage : uint16_t {
            CONSUMER_CONTROL        = 0x01
        };

        template<typename PAGE, typename USAGE>
        static uint32_t MakeUsage(const PAGE usagePage, const USAGE usage)
        {
            return ((static_cast<uint16_t>(usagePage) << 16) | static_cast<uint16_t>(usage));
        }

    public:
        class Collection;

        class Report {
        public:
            class Element {
            public:
                enum category : uint8_t {
                    INPUT = 1,
                    OUTPUT = 2,
                    FEATURE = 3
                };

            public:
                Element() = delete;
                Element(const Element&) = delete;
                Element& operator=(const Element&) = delete;
                Element(Report& parent, const category type,
                        const std::list<uint32_t>& usages, uint32_t flags,
                        const uint16_t count, const uint16_t size,
                        const uint32_t logicalMin, const uint32_t logicalMax)
                    : _report(parent)
                    , _type(type)
                    , _usages(usages)
                    , _count(count)
                    , _size(size)
                    , _logicalMin(logicalMin)
                    , _logicalMax(logicalMax)
                {
                }
                ~Element() = default;

            public:
                Report& Parent() const
                {
                    return (_report);
                }
                const category Type() const
                {
                    return (_type);
                }
                const std::list<uint32_t> Usages() const
                {
                    return (_usages);
                }
                uint16_t Size() const
                {
                    return (_size);
                }
                uint16_t Count() const
                {
                    return (_count);
                }
                uint32_t LogicalMin() const
                {
                    return (_logicalMin);
                }
                uint32_t LogicalMax() const
                {
                    return (_logicalMax);
                }

            private:
                Report& _report;
                category _type;
                std::list<uint32_t> _usages;
                uint16_t _count;
                uint16_t _size;
                uint32_t _logicalMin;
                uint32_t _logicalMax;
            }; // class Element

        public:
            Report() = delete;
            Report(const Report&) = delete;
            Report& operator=(const Report&) = delete;
            Report(Collection& parent, const uint8_t id = 0)
                : _collection(parent)
                , _id(id)
                , _elements()
            {
            }
            ~Report() = default;

        public:
            Collection& Parent() const
            {
                return (_collection);
            }
            uint8_t ID() const
            {
                return (_id);
            }
            const std::list<Element>& Elements() const
            {
                return (_elements);
            }

        private:
            friend class HID;

            template<typename... Args>
            Element& Add(Args&&... args)
            {
                _elements.emplace_back(*this, std::forward<Args>(args)...);
                return (_elements .back());
            }

        private:
            Collection& _collection;
            uint8_t _id;
            std::list<Element> _elements;
        }; // class Report

        class Collection {
        public:
            enum category : uint8_t {
                PHYSICAL = 0,
                APPLICATION = 1,
                LOGICAL = 2,
                ROOT = 0xFF
            };

        public:
            Collection(const Collection&) = delete;
            Collection& operator=(const Collection&) = delete;

            Collection()
                : _collection(nullptr)
                , _type(ROOT)
                , _usage(0)
            {
            }
            Collection(Collection& parent, const category type, const uint32_t usage)
                : _collection(&parent)
                , _type(type)
                , _usage(usage)
                , _collections()
                , _reports()
            {
            }
            ~Collection() = default;

        public:
            Collection& Parent() const
            {
                return (*_collection);
            }
            uint32_t Usage() const
            {
                return (_usage);
            }
            category Type() const
            {
                return (_type);
            }
            const std::list<Report>& Reports() const
            {
                return (_reports);
            }
            const std::list<Collection>& Collections() const
            {
                return (_collections);
            }

        private:
            friend class HID;

            template<typename... Args>
            Report& Add(Args&&... args)
            {
                _reports.emplace_back(*this, std::forward<Args>(args)...);
                return (_reports.back());
            }

            template<typename... Args>
            Collection& Child(Args&&... args)
            {
                _collections.emplace_back(*this, std::forward<Args>(args)...);
                return (_collections.back());
            }

        private:
            Collection* _collection;
            category _type;
            uint32_t _usage;
            std::list<Collection> _collections;
            std::list<Report> _reports;
        }; // class Collection

    public:
        HID(const HID&) = delete;
        HID& operator=(const HID&) = delete;

        HID()
            : _rootCollection()
        {
        }
        HID(const uint16_t length, const uint8_t data[])
            : _rootCollection()
        {
            Deserialize(length, data);
        }
        ~HID() = default;

    public:
        bool Deserialize(const uint16_t length, const uint8_t data[])
        {
            return (ParseReportMap(length, data, _rootCollection));
        }
        const Collection& ReportMap() const
        {
            return (_rootCollection);
        }

    private:
        static bool ParseReportMap(const uint16_t length, const uint8_t data[], Collection& rootCollection);

    private:
        Collection _rootCollection;
    }; // class HID

} // namespace USB

}
