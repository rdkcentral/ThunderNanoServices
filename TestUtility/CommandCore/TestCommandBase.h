#include "TestCommandMetadata.h"
#include "interfaces/ITestUtility.h"

#include "../Module.h"

namespace WPEFramework {

class TestCommandBase : public Exchange::ITestUtility::ICommand {
    public:
        TestCommandBase() = delete;
        TestCommandBase(const TestCommandBase&) = delete;
        TestCommandBase& operator=(const TestCommandBase&) = delete;

    public:
        class SignatureBuilder {
            public:
                SignatureBuilder() = delete;
                SignatureBuilder(const SignatureBuilder&) = delete;
                SignatureBuilder& operator=(const SignatureBuilder&) = delete;

            public:
                explicit SignatureBuilder(const TestCore::TestCommandSignature::Parameter& returnParam)
                    : _jsonSignature()
                {
                    _jsonSignature.Output = returnParam;

                }

                SignatureBuilder& InputParameter(const TestCore::TestCommandSignature::Parameter& inputParam)
                {
                    _jsonSignature.Input.Add(inputParam);

                    return (*this);
                }

                virtual ~SignatureBuilder() = default;

            private:

                friend class TestCommandBase;
                string ToString() const
                {
                    string outString;
                    _jsonSignature.ToString(outString);

                    return outString;
                }

                TestCore::TestCommandSignature _jsonSignature;
        };

        class DescriptionBuilder {
            public:
                DescriptionBuilder() = delete;
                DescriptionBuilder(const DescriptionBuilder&) = delete;
                DescriptionBuilder& operator=(const DescriptionBuilder&) = delete;

            public:
                explicit DescriptionBuilder(const string& description)
                    : _jsonDescription()
                {
                    _jsonDescription.Description = description;
                }

                virtual ~DescriptionBuilder() = default;

            private:

                friend class TestCommandBase;
                string ToString() const
                {
                    string outString;
                    _jsonDescription.ToString(outString);

                    return outString;
                }

                TestCore::TestCommandDescription _jsonDescription;
        };

    public:
        explicit TestCommandBase(const DescriptionBuilder& description, const SignatureBuilder& signature)
            : Exchange::ITestUtility::ICommand()
            , _description(description.ToString())
            , _signature(signature.ToString())
        { }

        virtual ~TestCommandBase() = default;

    public:
        // ICommand methods
        string Description() const final
        {
            return _description;
        }

        string Signature() const final
        {
            return _signature;
        }

    private:
        string _description;
        string _signature;
};
} // namespace WPEFramework
