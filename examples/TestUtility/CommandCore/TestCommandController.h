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
 
#pragma once

#include "../Module.h"

#include <interfaces/ITestUtility.h>

namespace WPEFramework {
namespace TestCore {

    class TestCommandController : virtual public Core::IUnknown {
    protected:
        TestCommandController()
            : _adminLock()
            , _commands()
        {
        }
    public:
        TestCommandController(const TestCommandController&) = delete;
        TestCommandController& operator=(const TestCommandController&) = delete;
    private:
        using TestCommandContainer = std::map<string, Exchange::ITestUtility::ICommand*>;
        using TestCommandContainerPair = std::pair<string, Exchange::ITestUtility::ICommand*>;

        class Iterator : public Exchange::ITestUtility::ICommand::IIterator {
        public:
            Iterator(const Iterator&) = delete;
            Iterator& operator=(const Iterator&) = delete;

        public:
            using IteratorImpl = Core::IteratorMapType<TestCommandContainer, Exchange::ITestUtility::ICommand*, string>;

            explicit Iterator(const TestCommandContainer& commands)
                : Exchange::ITestUtility::ICommand::IIterator()
                , _container(commands)
                , _iterator(_container)
            {
            }

            virtual ~Iterator() = default;

            BEGIN_INTERFACE_MAP(Iterator)
            INTERFACE_ENTRY(Exchange::ITestUtility::ICommand::IIterator)
            END_INTERFACE_MAP

            void Reset() override
            {
                _iterator.Reset(0);
            }

            bool IsValid() const override
            {
                return _iterator.IsValid();
            }

            bool Next() override
            {
                return _iterator.Next();
            }

            Exchange::ITestUtility::ICommand* Command() const override
            {
                return *_iterator;
            }

        private:
            TestCommandContainer _container;
            IteratorImpl _iterator;
        };
    public:
        static TestCommandController& Instance();
        ~TestCommandController();

        // ITestUtility methods
        Exchange::ITestUtility::ICommand* Command(const string& name);
        Exchange::ITestUtility::ICommand::IIterator* Commands(void) const;

        // TestCommandController methods
        void Announce(Exchange::ITestUtility::ICommand* command);
        void Revoke(Exchange::ITestUtility::ICommand* command);

        BEGIN_INTERFACE_MAP(TestCommandController)
        END_INTERFACE_MAP
    private:
        mutable Core::CriticalSection _adminLock;
        TestCommandContainer _commands;
    };
} // namespace TestCore
} // namespace WPEFramework
