#include "../IdentityProvider.h"

#include <nexus_config.h>
#include <nxclient.h>

#if NEXUS_SECURITY_API_VERSION == 2
#include <nexus_otp_key.h>
#else
#include <nexus_otpmsp.h>
#include <nexus_read_otp_id.h>
#endif

#if NEXUS_SECURITY_API_VERSION == 2

static class NexusId {
public:
    NexusId(const NexusId&) = delete;
    NexusId& operator= (const NexusId&) = delete;

    NexusId() : _length(0) {
    }
    ~NexusId() {
    }

    const unsigned char* Identifier(unsigned char* length) {
        if (_length == 0) {
        {
            if (NEXUS_SUCCESS == NxClient_Join(nullptr))
            {
                if (NEXUS_SUCCESS == NEXUS_OtpKey_GetInfo(0 /*key A*/, &_id)){
                    _length = NEXUS_OTP_KEY_ID_LENGTH;
                    if (length != nullptr) {
                        *length = _length;
                    }
                }
                else if (length != nullptr) {
                    *length = 2;
                }
                NxClient_Uninit();
            }
            else if (length != nullptr) {
                *length = 1;
            }
        }
        else if (length != nullptr) {
            *length = _length;
        }
        return (_length != 0 ? _id.id: nullptr);
    }
  
private: 
    unsigned char _length;
    NEXUS_OtpKeyInfo keyInfo;
} _identifier;

#else

static class NexusId {
public:
    NexusId(const NexusId&) = delete;
    NexusId& operator= (const NexusId&) = delete;

    NexusId() {
        _id.size = 0;
    }
    ~NexusId() {
    }

    const unsigned char* Identifier(unsigned char* length) {
        if (_id.size == 0)
        {
            if (NEXUS_SUCCESS == NxClient_Join(nullptr))
            {
                if (NEXUS_SUCCESS == NEXUS_Security_ReadOtpId(NEXUS_OtpIdType_eA, &_id)) {
                    if (length != nullptr) {
                        *length = static_cast<unsigned char>(_id.size);
                    }
                } else {
                    _id.size = 0;
                    if (length != nullptr) {
                        *length = 2;
                    }
                }
                NxClient_Uninit();
            }
            else if (length != nullptr) {
                *length = 1;
            }
        }
        else if (length != nullptr) {
            *length = static_cast<unsigned char>(_id.size);
        }
        return (_id.size != 0 ? _id.otpId : nullptr);
    }
  
private: 
    NEXUS_OtpIdOutput _id;
} _identifier;

#endif

const unsigned char* GetIdentity(unsigned char* length)
{
    return (_identifier.Identifier(length));
}
