#include "Module.h"

#include <interfaces/ITestController.h>

#include "Core/TestAdministrator.h"

namespace WPEFramework {
namespace TestCore {

    class TestControllerImp : public Exchange::ITestController {
    public:
        TestControllerImp() = default;
        TestControllerImp(const TestControllerImp&) = delete;
        TestControllerImp& operator=(const TestControllerImp&) = delete;

        virtual ~TestControllerImp() {}

        //  TestControllerImp methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Setup(){
            //This is overall TestController setup
            //ToDo: Do 'Setup' for ITestController if it is needed
        };
        virtual void TearDown(){
            //This is overall TestController tear down
            //ToDo: Do 'Tear Down' for ITestController if it is needed
        };

        virtual ICategory::IIterator* Categories() const override
        {
            return TestCore::TestAdministrator::Instance().Categories();
        }

        virtual ICategory* Category(const string& category) const override
        {
            return TestCore::TestAdministrator::Instance().Category(category);
        }

        BEGIN_INTERFACE_MAP(TestControllerImp)
        INTERFACE_ENTRY(Exchange::ITestController)
        END_INTERFACE_MAP
    private:
    };

    SERVICE_REGISTRATION(TestControllerImp, 1, 0);
} // namespace TestCore
} // namespace WPEFramewor
