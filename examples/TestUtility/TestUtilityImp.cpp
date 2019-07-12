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
        TestUtilityImp() {
        }

        virtual ~TestUtilityImp() {
            TestCore::TestCommandController::Instance().Release();
        };

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
    };

    SERVICE_REGISTRATION(TestUtilityImp, 1, 0);
} // namespace TestCore
} // namespace WPEFramewor
