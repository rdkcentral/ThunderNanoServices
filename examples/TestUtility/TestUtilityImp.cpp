#include "Module.h"

#include "CommandCore/TestCommandController.h"
#include <interfaces/ITestUtility.h>

namespace WPEFramework {
namespace TestCore {

    class TestUtilityImp : public Exchange::ITestUtility {
    public:
        TestUtilityImp(const TestUtilityImp&) = delete;
        TestUtilityImp& operator=(const TestUtilityImp&) = delete;

    public:
        TestUtilityImp()
        : _shutdownTimeout(0) {
        }

        virtual ~TestUtilityImp() {
            ::SleepMs(_shutdownTimeout);
            TestCore::TestCommandController::Instance().Release();
        };

        uint32_t ShutdownTimeout(const uint32_t timeout) override
        {
            _shutdownTimeout = timeout;
            return Core::ERROR_NONE;
        }

        //  ITestUtility methods
        // -------------------------------------------------------------------------------------------------------
        Exchange::ITestUtility::ICommand::IIterator* Commands() const override
        {
            return TestCore::TestCommandController::Instance().Commands();
        }

        Exchange::ITestUtility::ICommand* Command(const string& name) const override
        {
            return TestCore::TestCommandController::Instance().Command(name);
        }

        BEGIN_INTERFACE_MAP(TestUtilityImp)
        INTERFACE_ENTRY(Exchange::ITestUtility)
        END_INTERFACE_MAP

    private:
        uint32_t _shutdownTimeout;
    };

    SERVICE_REGISTRATION(TestUtilityImp, 1, 0);
} // namespace TestCore
} // namespace WPEFramewor
