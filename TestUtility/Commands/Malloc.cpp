#include "../CommandCore/TestCommandController.h"
#include "../CommandCore/TestCommandBase.h"
#include "MemoryAllocation.h"

namespace WPEFramework {

class Malloc : public TestCommandBase {
    public:
        Malloc(const Malloc&) = delete;
        Malloc& operator=(const Malloc&) = delete;

    private:
        class MallocInputMetadata : public Core::JSON::Container {
            public:
                MallocInputMetadata(const MallocInputMetadata&) = delete;
                MallocInputMetadata& operator=(const MallocInputMetadata&) = delete;

            public:
                MallocInputMetadata()
                    : Core::JSON::Container()
                    , Size(0)
                {
                    Add(_T("size"), &Size);
                }
                ~MallocInputMetadata() = default;

            public:
                Core::JSON::DecSInt32 Size;
        };

    public:
        using Parameter = TestCore::TestCommandSignature::Parameter;

        Malloc()
            : TestCommandBase(TestCommandBase::DescriptionBuilder("Allocates desired kB in memory and holds it"),
                              TestCommandBase::SignatureBuilder(Parameter("memory", Parameter::JSType::NUMBER, "memory statistics in KB"))
                              .InputParameter(Parameter("size", Parameter::JSType::NUMBER, "memory in kB for allocation")))
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
            MallocInputMetadata input;
            uint32_t size;

            if (input.FromString(params))
            {
                size = input.Size;
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
