#pragma once

#undef EXTERNAL
#define EXTERNAL __attribute__((visibility("default")))

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* cec_adapter_handle_t;

typedef enum cec_adapter_error {
    HDMI_CEC_ADAPTER_ERROR_OK = 0,
    HDMI_CEC_ADAPTER_ERROR_UNSUPORTED = -1,
    HDMI_CEC_ADAPTER_ERROR_SYSTEM = -2,
    HDMI_CEC_ADAPTER_ERROR_INVALID_ARGUMENT = -3,
    HDMI_CEC_ADAPTER_ERROR_UNAVAILABLE = -4,
    HDMI_CEC_ADAPTER_ERROR_GENERAL = -5,
    HDMI_CEC_ADAPTER_ERROR_EXHAUSTED = -6,
    HDMI_CEC_ADAPTER_ERROR_TIMEOUT = -7,
    HDMI_CEC_ADAPTER_ERROR_NOT_FOUND = -8,
    HDMI_CEC_ADAPTER_ERROR_INCONSITENT = -9,
    HDMI_CEC_ADAPTER_ERROR_INVALID_STATE = -10
} cec_adapter_error_t;

typedef enum cec_adapter_role {
    CEC_DEVICE_TV = 0x01,
    CEC_DEVICE_RECORDER = 0x02,
    CEC_DEVICE_TUNER = 0x04,
    CEC_DEVICE_PLAYBACK = 0x08,
    CEC_DEVICE_AUDIOSYSTEM = 0x10,
    CEC_DEVICE_SWITCH = 0x20,
    CEC_DEVICE_VIDEOPROCESSOR = 0x40,
    /* Reserved */
    CEC_DEVICE_UNKNOWN = 0x00,
    CEC_DEVICE_INTERNAL = 0xFD,
    CEC_DEVICE_ALL = 0x7F,
} cec_adapter_role_t;

typedef struct cec_adapter_callbacks {
    void (*received)(void* cb_data,
                     const cec_adapter_role_t follower, const uint8_t initiator,
                     const uint8_t length, const uint8_t payload[]);
    void (*claimed_roles)(void* cb_data, const uint8_t rolesMask);
} cec_adapter_callbacks_t;

cec_adapter_handle_t cec_adapter_create(const char connector[]);
EXTERNAL cec_adapter_error_t cec_adapter_destroy(cec_adapter_handle_t handle);

EXTERNAL cec_adapter_error_t cec_adapter_receive(cec_adapter_handle_t handle, cec_adapter_callbacks_t* callbacks, void* cb_data);
EXTERNAL cec_adapter_error_t cec_adapter_transmit(cec_adapter_handle_t handle,
                                                  const cec_adapter_role_t initiator, const uint8_t follower,
                                                  const uint8_t length, const uint8_t data[]);

EXTERNAL cec_adapter_error_t cec_adapter_claim_role(cec_adapter_handle_t handle, const cec_adapter_role_t role);
EXTERNAL cec_adapter_error_t cec_adapter_release_role(cec_adapter_handle_t handle, const cec_adapter_role_t role);

#ifdef __cplusplus
}
#endif