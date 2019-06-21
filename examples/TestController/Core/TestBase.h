#include "interfaces/ITestController.h"

#include "../Module.h"
#include "TestMetadata.h"

namespace WPEFramework {

class TestBase : public Exchange::ITestController::ITest {
public:
    TestBase(const TestBase&) = delete;
    TestBase& operator=(const TestBase&) = delete;

    class DescriptionBuilder {
    public:
        DescriptionBuilder(const DescriptionBuilder&) = delete;
        DescriptionBuilder& operator=(const DescriptionBuilder&) = delete;

        explicit DescriptionBuilder(const string& description)
            : _jsonDescription()
        {
            _jsonDescription.Description = description;
        }

        virtual ~DescriptionBuilder() = default;

    private:
        friend class TestBase;
        string ToString() const
        {
            string outString;
            _jsonDescription.ToString(outString);

            return outString;
        }

        TestCore::TestDescription _jsonDescription;
    };

    explicit TestBase(const DescriptionBuilder& description)
        : Exchange::ITestController::ITest()
        , _description(description.ToString())
    {
    }

    virtual ~TestBase() = default;

    // ICommand methods
    string Description() const final
    {
        return _description;
    }

    BEGIN_INTERFACE_MAP(TestBase)
    INTERFACE_ENTRY(Exchange::ITestController::ITest)
    END_INTERFACE_MAP

private:
    string _description;
};
} // namespace WPEFramework
