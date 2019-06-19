#include <IdentityProvider.h>

#include <nexus_config.h>
#include <nxclient.h>

#if NEXUS_SECURITY_API_VERSION == 2
#include <nexus_otp_key.h>
#else
#include <nexus_otpmsp.h>
#include <nexus_read_otp_id.h>
#endif

namespace WPEFramework {
namespace Plugin {
IdentityProvider::IdentityProvider(): _identifier(nullptr)
    {
        ASSERT(_identifier == nullptr);
        if (_identifier == nullptr) {
            if (NEXUS_SUCCESS == NxClient_Join(NULL))
            {
#if NEXUS_SECURITY_API_VERSION == 2
                NEXUS_OtpKeyInfo keyInfo;
                if (NEXUS_SUCCESS == NEXUS_OtpKey_GetInfo(0 /*key A*/, &keyInfo)){
                    _identifier = new uint8_t[NEXUS_OTP_KEY_ID_LENGTH + 2];

                    ::memcpy(&(_identifier[1]), keyInfo.id, NEXUS_OTP_KEY_ID_LENGTH);

                    _identifier[0] = NEXUS_OTP_KEY_ID_LENGTH;
                    _identifier[NEXUS_OTP_KEY_ID_LENGTH + 1] = '\0';
                }
#else 
                NEXUS_OtpIdOutput id;
                if (NEXUS_SUCCESS == NEXUS_Security_ReadOtpId(NEXUS_OtpIdType_eA, &id) ) {
                    _identifier = new uint8_t[id.size + 2];

                    ::memcpy(&(_identifier[1]), id.otpId, id.size);

                    _identifier[0] = id.size;
                    _identifier[id.size + 1] = '\0';
                }
#endif // NEXUS_SECURITY_API_VERSION
                NxClient_Uninit();
            }
        }
    }
} // namespace Plugin
} // namespace WPEFramework