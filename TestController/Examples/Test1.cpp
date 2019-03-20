#include "../Module.h"

#include "../Core/TestBase.h"
#include "../Core/Trace.h"
#include "TestCategory1.h"
#include <interfaces/ITestController.h>

namespace WPEFramework {

class Test1 : public TestBase {
public:
    Test1(const Test1&) = delete;
    Test1& operator=(const Test1&) = delete;

    Test1()
        : TestBase(TestBase::DescriptionBuilder("Test 1 description"))
    {
        TestCore::TestCategory1::Instance().Register(this);
    }

    virtual ~Test1()
    {
        TestCore::TestCategory1::Instance().Unregister(this);
    }

public:
    // ICommand methods
    string Execute(const string& params) final
    {
        TestCore::TestResult jsonResult;
        string result;
        TRACE(TestCore::TestStart, (_T("Start execute of test: %s"), _name.c_str()));

        TRACE(TestCore::TestStep, (_T("Do real test execution"), _name.c_str()));
        jsonResult.Name = _name;
        jsonResult.OverallStatus = "Success";

        TRACE(TestCore::TestStart, (_T("End test: %s"), _name.c_str()));
        jsonResult.ToString(result);
        return result;
    }

    string Name() const final
    {
        return _name;
    }

private:
    const string _name = _T("Test1");
};

static Exchange::ITestController::ITest* _singleton(Core::Service<Test1>::Create<Exchange::ITestController::ITest>());
} // namespace WPEFramework
