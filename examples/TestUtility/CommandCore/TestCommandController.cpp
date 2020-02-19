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
 
#include "TestCommandController.h"

namespace WPEFramework {
namespace TestCore {

    /* static */ TestCommandController& TestCommandController::Instance()
    {
        static TestCommandController*_singleton(Core::Service<TestCommandController>::Create<TestCommandController>());;
        return (*_singleton);
    }

    TestCommandController::~TestCommandController() {
        _adminLock.Lock();
        Exchange::ITestUtility::ICommand::IIterator* iterator = Commands();
        while(iterator->Next()) {
            if (iterator->Command()->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
                TRACE(Trace::Information, (_T("Command %s in not properly destructed!"), iterator->Command()->Name().c_str()));
            }
        }
        iterator->Release();
        _adminLock.Unlock();
    };

    void TestCommandController::Announce(Exchange::ITestUtility::ICommand* command)
    {
        ASSERT(command != nullptr);

        _adminLock.Lock();
        std::pair<TestCommandContainer::iterator, bool> returned = _commands.insert(TestCommandContainerPair(command->Name(), command));
        ASSERT((returned.second == true) && "Test command already exists!");
        _adminLock.Unlock();
    }

    void TestCommandController::Revoke(Exchange::ITestUtility::ICommand* command)
    {
        ASSERT(command != nullptr);

        _adminLock.Lock();
        _commands.erase(command->Name());
        _adminLock.Unlock();
    }

    Exchange::ITestUtility::ICommand* TestCommandController::Command(const string& name)
    {
        Exchange::ITestUtility::ICommand* command = nullptr;

        _adminLock.Lock();
        Exchange::ITestUtility::ICommand::IIterator* iter = Commands();

        while (iter->Next()) {
            if (iter->Command()->Name() == name) {
                command = iter->Command();
                break;
            }
        }
        _adminLock.Unlock();

        return command;
    }

    Exchange::ITestUtility::ICommand::IIterator* TestCommandController::Commands(void) const
    {
        Exchange::ITestUtility::ICommand::IIterator* iterator = nullptr;
        _adminLock.Lock();
        iterator = Core::Service<Iterator>::Create<Exchange::ITestUtility::ICommand::IIterator>(_commands);
        _adminLock.Unlock();
        return iterator;
    }
} // namespace TestCore
} // namespace WPEFramework
