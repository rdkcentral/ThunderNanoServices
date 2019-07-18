#include "../CommandCore/TestCommandBase.h"
#include "../CommandCore/TestCommandController.h"
#include "MemoryAllocation.h"

namespace WPEFramework {

class Free : public TestCommandBase {
public:
    Free(const Free&) = delete;
    Free& operator=(const Free&) = delete;

public:
    using Parameter = JsonData::TestUtility::InputInfo;

    Free()
        : TestCommandBase(TestCommandBase::DescriptionBuilder("Releases previously allocated memory"),
              TestCommandBase::SignatureBuilder("memory", Parameter::TypeType::NUMBER, "memory statistics in KB"))
        , _memoryAdmin(MemoryAllocation::Instance())
    {
        TestCore::TestCommandController::Instance().Announce(this);
    }

    virtual ~Free()
    {
        TestCore::TestCommandController::Instance().Revoke(this);
    }

public:
    // ICommand methods
    string Execute(const string& params) final
    {
        bool status = _memoryAdmin.Free();
        return (status == true ? _memoryAdmin.CreateResponse() : EMPTY_STRING);
    }

    string Name() const final
    {
        return _name;
    }

private:
    BEGIN_INTERFACE_MAP(Free)
    INTERFACE_ENTRY(Exchange::ITestUtility::ICommand)
    END_INTERFACE_MAP

private:
    MemoryAllocation& _memoryAdmin;
    const string _name = _T("Free");
};

static Free* _singleton(Core::Service<Free>::Create<Free>());

} // namespace WPEFramework
