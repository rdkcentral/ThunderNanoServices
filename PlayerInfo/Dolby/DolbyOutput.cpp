#include "Dolby.h"

namespace WPEFramework
{
    namespace Plugin
    {

        class DolbyOutputImplementation : public Exchange::Dolby::IOutput
        {
        public:
            void Mode(const Exchange::Dolby::IOutput::Type value) override
            {
                uint32_t errorCode = set_audio_output_type(value);

                ASSERT(errorCode == Core::ERROR_NONE);
            };

            Exchange::Dolby::IOutput::Type Mode() const override
            {
                uint32_t errorCode = Core::ERROR_NONE;
                Exchange::Dolby::IOutput::Type result = get_audio_output_type(errorCode);

                ASSERT(errorCode == Core::ERROR_NONE);
            };

            BEGIN_INTERFACE_MAP(DolbyOutputImplementation)
                INTERFACE_ENTRY(Exchange::Dolby::IOutput)
            END_INTERFACE_MAP
        };

        SERVICE_REGISTRATION(DolbyOutputImplementation, 1, 0);

    } // namespace Plugin
} // namespace WPEFramework