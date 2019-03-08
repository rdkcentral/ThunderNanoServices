#include "../CommandCore/TestCommandBase.h"
#include "../CommandCore/TestCommandController.h"

#include "CrashCore.h"

namespace WPEFramework {

class CrashNTimes : public TestCommandBase {
public:
    CrashNTimes(const CrashNTimes&) = delete;
    CrashNTimes& operator=(const CrashNTimes&) = delete;

public:
    using Parameter = TestCore::TestCommandSignature::Parameter;

    CrashNTimes()
        : TestCommandBase(TestCommandBase::DescriptionBuilder(_T("Cause segmenation fault N times in a row")),
              TestCommandBase::SignatureBuilder(Parameter())
                  .InputParameter(Parameter("crashCount", Parameter::JSType::NUMBER, "how many times Crash will be executed consecutively")))
        , _crashCore(CrashCore::Instance())
        , _name(_T("CrashNTimes"))
    {
        TestCore::TestCommandController::Instance().Announce(this);

        _crashCore.ExecPendingCrash();
    }

    virtual ~CrashNTimes() { TestCore::TestCommandController::Instance().Revoke(this); }

    BEGIN_INTERFACE_MAP(CrashNTimes)
    INTERFACE_ENTRY(Exchange::ITestUtility::ICommand)
    END_INTERFACE_MAP

private:
    class CrashNTimesInputMetadata : public Core::JSON::Container {
    public:
        CrashNTimesInputMetadata(const CrashNTimesInputMetadata&) = delete;
        CrashNTimesInputMetadata& operator=(const CrashNTimesInputMetadata&) = delete;

    public:
        CrashNTimesInputMetadata()
            : Core::JSON::Container()
            , CrashCount(0)
        {
            Add(_T("crashCount"), &CrashCount);
        }

        ~CrashNTimesInputMetadata() = default;

    public:
        Core::JSON::DecUInt8 CrashCount;
    };

public:
    virtual string Execute(const string& params) final
    {
        CrashNTimesInputMetadata input;
        string responseString = _T("");
        uint8_t crashCount = 0;

        if (input.FromString(params))
        {
            crashCount = input.CrashCount;
        }

        if (crashCount > 0)
        {
            responseString = _crashCore.CrashNTimes(crashCount);
        }
        else
        {
            responseString = _T("crashCount must be greater than 0");
        }

        return (_crashCore.ErrorResponse(responseString));
    }

    virtual string Name() const final { return _name; }

private:
    CrashCore& _crashCore;
    string _name;
};

static CrashNTimes* _singleton(Core::Service<CrashNTimes>::Create<CrashNTimes>());

} // namespace WPEFramework
