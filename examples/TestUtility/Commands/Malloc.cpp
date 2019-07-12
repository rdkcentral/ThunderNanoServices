#include "../CommandCore/TestCommandBase.h"
#include "../CommandCore/TestCommandController.h"
#include "MemoryAllocation.h"

namespace WPEFramework {

class Malloc : public TestCommandBase {
public:
    Malloc(const Malloc&) = delete;
    Malloc& operator=(const Malloc&) = delete;

public:
    using Parameter = JsonData::TestUtility::InputInfo;

    Malloc()
        : TestCommandBase(TestCommandBase::DescriptionBuilder("Allocates desired kB in memory and holds it"),
              TestCommandBase::SignatureBuilder("memory", Parameter::TypeType::NUMBER, "memory statistics in KB")
                  .InputParameter("size", Parameter::TypeType::NUMBER, "memory in kB for allocation"))
        , _memoryAdmin(MemoryAllocation::Instance())
    {
        TestCore::TestCommandController::Instance().Announce(this);
    }

    virtual ~Malloc()
    {
        TestCore::TestCommandController::Instance().Revoke(this);
    }

public:
    // ICommand methods
    string Execute(const string& params) final
    {
        JsonData::TestUtility::RunmemoryParamsData input;
        uint32_t size;

        if ((input.FromString(params) == true) && (input.Size.IsSet())) {
            size = input.Size.Value();
            _memoryAdmin.Malloc(size);
        }
        return _memoryAdmin.CreateResponse();
    }

    string Name() const final
    {
        return _name;
    }

private:
    BEGIN_INTERFACE_MAP(Malloc)
    INTERFACE_ENTRY(Exchange::ITestUtility::ICommand)
    END_INTERFACE_MAP

private:
    MemoryAllocation& _memoryAdmin;
    const string _name = _T("Malloc");
};

static Malloc* _singleton(Core::Service<Malloc>::Create<Malloc>());

} // namespace WPEFramework
