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
