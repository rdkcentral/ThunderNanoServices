#include "../CommandCore/TestCommandBase.h"
#include "../CommandCore/TestCommandController.h"

#include "CrashCore.h"

namespace WPEFramework {

class Crash : public TestCommandBase {
public:
    Crash(const Crash&) = delete;
    Crash& operator=(const Crash&) = delete;

public:
    using Parameter = TestCore::TestCommandSignature::Parameter;

    Crash()
        : TestCommandBase(
            TestCommandBase::DescriptionBuilder(_T("Cause segmenation fault resulting in crash")),
            TestCommandBase::SignatureBuilder(Parameter())
                .InputParameter(Parameter("crashDelay", Parameter::JSType::NUMBER, "delay in ms before actual crash")))
        , _crashCore(CrashCore::Instance())
        , _name(_T("Crash"))
    {
        TestCore::TestCommandController::Instance().Announce(this);
    }

    virtual ~Crash() { TestCore::TestCommandController::Instance().Revoke(this); }

    BEGIN_INTERFACE_MAP(Crash)
    INTERFACE_ENTRY(Exchange::ITestUtility::ICommand)
    END_INTERFACE_MAP

private:
    class CrashInputMetadata : public Core::JSON::Container {
    public:
        CrashInputMetadata(const CrashInputMetadata&) = delete;
        CrashInputMetadata& operator=(const CrashInputMetadata&) = delete;

    public:
        CrashInputMetadata()
            : Core::JSON::Container()
            , CrashDelay(0)
        {
            Add(_T("crashDelay"), &CrashDelay);
        }
        ~CrashInputMetadata() = default;

    public:
        Core::JSON::DecUInt8 CrashDelay;
    };

public:
    virtual string /*JSON*/ Execute(const string& params) final
    {
        CrashInputMetadata input;
        uint8_t crashDelay = CrashCore::DefaultCrashDelay;
        string responseString = _T("");

        if (input.FromString(params))
        {
            crashDelay = input.CrashDelay;
        }

        responseString = _crashCore.Crash(crashDelay);

        return (_crashCore.ErrorResponse(responseString));
    }

    virtual string Name() const final { return _name; }

private:
    CrashCore& _crashCore;
    string _name;
};

static Crash* _singleton(Core::Service<Crash>::Create<Crash>());

} // namespace WPEFramework
