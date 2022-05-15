#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <unordered_map>

#include <cec_device_adapter.h>

#undef EXTERNAL
#ifdef _MSVC_LANG
#ifdef CEC_ADAPTER_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "cec_adapter.lib")
#endif
#else
#define EXTERNAL __attribute__((visibility("default")))
#endif

// inspired by https://github.com/Pulse-Eight/libcec/blob/master/include/cectypes.h

namespace WPEFramework {
namespace CEC {
    typedef enum logical_address_type : uint8_t {
        CEC_LOGICAL_ADDRESS_TV = 0x00,
        CEC_LOGICAL_ADDRESS_RECORD_1 = 0x01,
        CEC_LOGICAL_ADDRESS_RECORD_2 = 0x02,
        CEC_LOGICAL_ADDRESS_TUNER_1 = 0x03,
        CEC_LOGICAL_ADDRESS_PLAYBACK_1 = 0x04,
        CEC_LOGICAL_ADDRESS_AUDIOSYSTEM = 0x05,
        CEC_LOGICAL_ADDRESS_TUNER_2 = 0x06,
        CEC_LOGICAL_ADDRESS_TUNER_3 = 0x07,
        CEC_LOGICAL_ADDRESS_PLAYBACK_2 = 0x08,
        CEC_LOGICAL_ADDRESS_RECORD_3 = 0x09,
        CEC_LOGICAL_ADDRESS_TUNER_4 = 0x0A,
        CEC_LOGICAL_ADDRESS_PLAYBACK_3 = 0x0B,
        CEC_LOGICAL_ADDRESS_BACKUP_1 = 0x0C,
        CEC_LOGICAL_ADDRESS_BACKUP_2 = 0x0D,
        CEC_LOGICAL_ADDRESS_SPECIFIC = 0x0E,
        CEC_LOGICAL_ADDRESS_UNREGISTERED = 0x0F,
        CEC_LOGICAL_ADDRESS_BROADCAST = 0x0F,
        CEC_LOGICAL_ADDRESS_INVALID = 0xFF
    } logical_address_t;

    typedef std::list<cec_adapter_role_t> device_list_t;
    typedef std::list<logical_address_t> logical_address_list_t;

    typedef std::map<cec_adapter_role_t, logical_address_t> role_map_t;

    typedef uint8_t role_mask_t;

    inline cec_adapter_role_t Convert(const logical_address_t address)
    {
        cec_adapter_role_t deviceType(CEC_DEVICE_UNKNOWN);

        switch (address) {
        case CEC_LOGICAL_ADDRESS_TV: {
            deviceType = CEC_DEVICE_TV;
            break;
        }
        case CEC_LOGICAL_ADDRESS_RECORD_1:
        case CEC_LOGICAL_ADDRESS_RECORD_2:
        case CEC_LOGICAL_ADDRESS_RECORD_3: {
            deviceType = CEC_DEVICE_RECORDER;
            break;
        }
        case CEC_LOGICAL_ADDRESS_TUNER_1:
        case CEC_LOGICAL_ADDRESS_TUNER_2:
        case CEC_LOGICAL_ADDRESS_TUNER_3:
        case CEC_LOGICAL_ADDRESS_TUNER_4: {
            deviceType = CEC_DEVICE_TUNER;
            break;
        }
        case CEC_LOGICAL_ADDRESS_PLAYBACK_1:
        case CEC_LOGICAL_ADDRESS_PLAYBACK_2:
        case CEC_LOGICAL_ADDRESS_PLAYBACK_3: {
            deviceType = CEC_DEVICE_PLAYBACK;
            break;
        }
        case CEC_LOGICAL_ADDRESS_AUDIOSYSTEM: {
            deviceType = CEC_DEVICE_AUDIOSYSTEM;
            break;
        }
        case CEC_LOGICAL_ADDRESS_BACKUP_1:
        case CEC_LOGICAL_ADDRESS_BACKUP_2: {
            deviceType = CEC_DEVICE_VIDEOPROCESSOR;
            break;
        }
        case CEC_LOGICAL_ADDRESS_SPECIFIC: {
            deviceType = CEC_DEVICE_SWITCH;
            break;
        }
        case CEC_LOGICAL_ADDRESS_BROADCAST: {
            deviceType = CEC_DEVICE_ALL;
            break;
        }
        default:
            break;
        }
        return deviceType;
    }

    inline logical_address_list_t Convert(const cec_adapter_role_t role)
    {
        logical_address_list_t addresses;

        switch (role) {
        case CEC_DEVICE_TV: {
            addresses = { CEC_LOGICAL_ADDRESS_TV };
            break;
        }
        case CEC_DEVICE_RECORDER: {
            addresses = { CEC_LOGICAL_ADDRESS_RECORD_1, CEC_LOGICAL_ADDRESS_RECORD_2, CEC_LOGICAL_ADDRESS_RECORD_3 };
            break;
        }
        case CEC_DEVICE_TUNER: {
            addresses = { CEC_LOGICAL_ADDRESS_TUNER_1, CEC_LOGICAL_ADDRESS_TUNER_2, CEC_LOGICAL_ADDRESS_TUNER_3, CEC_LOGICAL_ADDRESS_TUNER_4 };
            break;
        }
        case CEC_DEVICE_PLAYBACK: {
            addresses = { CEC_LOGICAL_ADDRESS_PLAYBACK_1, CEC_LOGICAL_ADDRESS_PLAYBACK_2, CEC_LOGICAL_ADDRESS_PLAYBACK_3 };
            break;
        }
        case CEC_DEVICE_SWITCH: {
            addresses = { CEC_LOGICAL_ADDRESS_SPECIFIC };
            break;
        }
        case CEC_DEVICE_VIDEOPROCESSOR: {
            addresses = { CEC_LOGICAL_ADDRESS_BACKUP_1, CEC_LOGICAL_ADDRESS_BACKUP_2 };
            break;
        }
        default:
            break;
        }

        return addresses;
    }

    typedef enum opcode_type : uint8_t {
        ACTIVE_SOURCE = 0x82,
        IMAGE_VIEW_ON = 0x04,
        TEXT_VIEW_ON = 0x0d,
        INACTIVE_SOURCE = 0x9d,
        REQUEST_ACTIVE_SOURCE = 0x85,
        ROUTING_CHANGE = 0x80,
        ROUTING_INFORMATION = 0x81,
        SET_STREAM_PATH = 0x86,
        STANDBY = 0x36,
        RECORD_OFF = 0x0b,
        RECORD_ON = 0x09,
        RECORD_STATUS = 0x0a,
        RECORD_TV_SCREEN = 0x0f,
        CLEAR_ANALOGUE_TIMER = 0x33,
        CLEAR_DIGITAL_TIMER = 0x99,
        CLEAR_EXT_TIMER = 0xa1,
        SET_ANALOGUE_TIMER = 0x34,
        SET_DIGITAL_TIMER = 0x97,
        SET_EXT_TIMER = 0xa2,
        SET_TIMER_PROGRAM_TITLE = 0x67,
        TIMER_CLEARED_STATUS = 0x43,
        TIMER_STATUS = 0x35,
        CEC_VERSION = 0x9e,
        GET_CEC_VERSION = 0x9f,
        GIVE_PHYSICAL_ADDR = 0x83,
        GET_MENU_LANGUAGE = 0x91,
        REPORT_PHYSICAL_ADDR = 0x84,
        SET_MENU_LANGUAGE = 0x32,
        REPORT_FEATURES = 0xa6 /* HDMI 2.0 */,
        GIVE_FEATURES = 0xa5 /* HDMI 2.0 */,
        DECK_CONTROL = 0x42,
        DECK_STATUS = 0x1b,
        GIVE_DECK_STATUS = 0x1a,
        PLAY = 0x41,
        GIVE_TUNER_DEVICE_STATUS = 0x08,
        SELECT_ANALOGUE_SERVICE = 0x92,
        SELECT_DIGITAL_SERVICE = 0x93,
        TUNER_DEVICE_STATUS = 0x07,
        TUNER_STEP_DECREMENT = 0x06,
        TUNER_STEP_INCREMENT = 0x05,
        DEVICE_VENDOR_ID = 0x87,
        GIVE_DEVICE_VENDOR_ID = 0x8c,
        VENDOR_COMMAND = 0x89,
        VENDOR_COMMAND_WITH_ID = 0xa0,
        VENDOR_REMOTE_BUTTON_DOWN = 0x8a,
        VENDOR_REMOTE_BUTTON_UP = 0x8b,
        SET_OSD_STRING = 0x64,
        GIVE_OSD_NAME = 0x46,
        SET_OSD_NAME = 0x47,
        MENU_REQUEST = 0x8d,
        MENU_STATUS = 0x8e,
        USER_CONTROL_PRESSED = 0x44,
        USER_CONTROL_RELEASED = 0x45,
        GIVE_DEVICE_POWER_STATUS = 0x8f,
        REPORT_POWER_STATUS = 0x90,
        FEATURE_ABORT = 0x00,
        ABORT = 0xff,
        GIVE_AUDIO_STATUS = 0x71,
        GIVE_SYSTEM_AUDIO_MODE_STATUS = 0x7d,
        REPORT_AUDIO_STATUS = 0x7a,
        REPORT_SHORT_AUDIO_DESCRIPTOR = 0xa3,
        REQUEST_SHORT_AUDIO_DESCRIPTOR = 0xa4,
        SET_SYSTEM_AUDIO_MODE = 0x72,
        SYSTEM_AUDIO_MODE_REQUEST = 0x70,
        SYSTEM_AUDIO_MODE_STATUS = 0x7e,
        SET_AUDIO_RATE = 0x9a,
        INITIATE_ARC = 0xc0,
        REPORT_ARC_INITIATED = 0xc1,
        REPORT_ARC_TERMINATED = 0xc2,
        REQUEST_ARC_INITIATION = 0xc3,
        REQUEST_ARC_TERMINATION = 0xc4,
        TERMINATE_ARC = 0xc5,
        REQUEST_CURRENT_LATENCY = 0xa7,
        REPORT_CURRENT_LATENCY = 0xa8,
        CDC_MESSAGE = 0xf8,
        NO_OPCODE = 0xFD /* when this opcode is set, no opcode will be sent to the device. this is one of the reserved numbers */
    } opcode_t;

    typedef enum abort_reason_type : uint8_t {
        CEC_ABORT_REASON_UNRECOGNIZED_OPCODE = 0,
        CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND = 1,
        CEC_ABORT_REASON_CANNOT_PROVIDE_SOURCE = 2,
        CEC_ABORT_REASON_INVALID_OPERAND = 3,
        CEC_ABORT_REASON_REFUSED = 4
    } abort_reason_t;

    typedef enum version_type : uint8_t {
        CEC_VERSION_UNKNOWN = 0x00,
        CEC_VERSION_1_2 = 0x01,
        CEC_VERSION_1_2A = 0x02,
        CEC_VERSION_1_3 = 0x03,
        CEC_VERSION_1_3A = 0x04,
        CEC_VERSION_1_4 = 0x05,
        CEC_VERSION_2_0 = 0x06,
    } version_t;

    typedef enum power_status_type : uint8_t {
        CEC_POWER_STATUS_ON = 0x00,
        CEC_POWER_STATUS_STANDBY = 0x01,
        CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON = 0x02,
        CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY = 0x03,
        CEC_POWER_STATUS_UNKNOWN = 0x99
    } power_status_t;

    typedef enum usercontrol_code_type : uint8_t {
        CEC_USER_CONTROL_CODE_SELECT = 0x00,
        CEC_USER_CONTROL_CODE_UP = 0x01,
        CEC_USER_CONTROL_CODE_DOWN = 0x02,
        CEC_USER_CONTROL_CODE_LEFT = 0x03,
        CEC_USER_CONTROL_CODE_RIGHT = 0x04,
        CEC_USER_CONTROL_CODE_RIGHT_UP = 0x05,
        CEC_USER_CONTROL_CODE_RIGHT_DOWN = 0x06,
        CEC_USER_CONTROL_CODE_LEFT_UP = 0x07,
        CEC_USER_CONTROL_CODE_LEFT_DOWN = 0x08,
        CEC_USER_CONTROL_CODE_ROOT_MENU = 0x09,
        CEC_USER_CONTROL_CODE_SETUP_MENU = 0x0A,
        CEC_USER_CONTROL_CODE_CONTENTS_MENU = 0x0B,
        CEC_USER_CONTROL_CODE_FAVORITE_MENU = 0x0C,
        CEC_USER_CONTROL_CODE_EXIT = 0x0D,
        // reserved: 0x0E, 0x0F
        CEC_USER_CONTROL_CODE_TOP_MENU = 0x10,
        CEC_USER_CONTROL_CODE_DVD_MENU = 0x11,
        // reserved: 0x12 ... 0x1C
        CEC_USER_CONTROL_CODE_NUMBER_ENTRY_MODE = 0x1D,
        CEC_USER_CONTROL_CODE_NUMBER11 = 0x1E,
        CEC_USER_CONTROL_CODE_NUMBER12 = 0x1F,
        CEC_USER_CONTROL_CODE_NUMBER0 = 0x20,
        CEC_USER_CONTROL_CODE_NUMBER1 = 0x21,
        CEC_USER_CONTROL_CODE_NUMBER2 = 0x22,
        CEC_USER_CONTROL_CODE_NUMBER3 = 0x23,
        CEC_USER_CONTROL_CODE_NUMBER4 = 0x24,
        CEC_USER_CONTROL_CODE_NUMBER5 = 0x25,
        CEC_USER_CONTROL_CODE_NUMBER6 = 0x26,
        CEC_USER_CONTROL_CODE_NUMBER7 = 0x27,
        CEC_USER_CONTROL_CODE_NUMBER8 = 0x28,
        CEC_USER_CONTROL_CODE_NUMBER9 = 0x29,
        CEC_USER_CONTROL_CODE_DOT = 0x2A,
        CEC_USER_CONTROL_CODE_ENTER = 0x2B,
        CEC_USER_CONTROL_CODE_CLEAR = 0x2C,
        CEC_USER_CONTROL_CODE_NEXT_FAVORITE = 0x2F,
        CEC_USER_CONTROL_CODE_CHANNEL_UP = 0x30,
        CEC_USER_CONTROL_CODE_CHANNEL_DOWN = 0x31,
        CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL = 0x32,
        CEC_USER_CONTROL_CODE_SOUND_SELECT = 0x33,
        CEC_USER_CONTROL_CODE_INPUT_SELECT = 0x34,
        CEC_USER_CONTROL_CODE_DISPLAY_INFORMATION = 0x35,
        CEC_USER_CONTROL_CODE_HELP = 0x36,
        CEC_USER_CONTROL_CODE_PAGE_UP = 0x37,
        CEC_USER_CONTROL_CODE_PAGE_DOWN = 0x38,
        // reserved: 0x39 ... 0x3F
        CEC_USER_CONTROL_CODE_POWER = 0x40,
        CEC_USER_CONTROL_CODE_VOLUME_UP = 0x41,
        CEC_USER_CONTROL_CODE_VOLUME_DOWN = 0x42,
        CEC_USER_CONTROL_CODE_MUTE = 0x43,
        CEC_USER_CONTROL_CODE_PLAY = 0x44,
        CEC_USER_CONTROL_CODE_STOP = 0x45,
        CEC_USER_CONTROL_CODE_PAUSE = 0x46,
        CEC_USER_CONTROL_CODE_RECORD = 0x47,
        CEC_USER_CONTROL_CODE_REWIND = 0x48,
        CEC_USER_CONTROL_CODE_FAST_FORWARD = 0x49,
        CEC_USER_CONTROL_CODE_EJECT = 0x4A,
        CEC_USER_CONTROL_CODE_FORWARD = 0x4B,
        CEC_USER_CONTROL_CODE_BACKWARD = 0x4C,
        CEC_USER_CONTROL_CODE_STOP_RECORD = 0x4D,
        CEC_USER_CONTROL_CODE_PAUSE_RECORD = 0x4E,
        // reserved: 0x4F
        CEC_USER_CONTROL_CODE_ANGLE = 0x50,
        CEC_USER_CONTROL_CODE_SUB_PICTURE = 0x51,
        CEC_USER_CONTROL_CODE_VIDEO_ON_DEMAND = 0x52,
        CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE = 0x53,
        CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING = 0x54,
        CEC_USER_CONTROL_CODE_INITIAL_CONFIGURATION = 0x55,
        CEC_USER_CONTROL_CODE_SELECT_BROADCAST_TYPE = 0x56,
        CEC_USER_CONTROL_CODE_SELECT_SOUND_PRESENTATION = 0x57,
        // reserved: 0x58 ... 0x5F
        CEC_USER_CONTROL_CODE_PLAY_FUNCTION = 0x60,
        CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION = 0x61,
        CEC_USER_CONTROL_CODE_RECORD_FUNCTION = 0x62,
        CEC_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION = 0x63,
        CEC_USER_CONTROL_CODE_STOP_FUNCTION = 0x64,
        CEC_USER_CONTROL_CODE_MUTE_FUNCTION = 0x65,
        CEC_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION = 0x66,
        CEC_USER_CONTROL_CODE_TUNE_FUNCTION = 0x67,
        CEC_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION = 0x68,
        CEC_USER_CONTROL_CODE_SELECT_AV_INPUT_FUNCTION = 0x69,
        CEC_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION = 0x6A,
        CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION = 0x6B,
        CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION = 0x6C,
        CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION = 0x6D,
        // reserved: 0x6E ... 0x70
        CEC_USER_CONTROL_CODE_F1_BLUE = 0x71,
        CEC_USER_CONTROL_CODE_F2_RED = 0X72,
        CEC_USER_CONTROL_CODE_F3_GREEN = 0x73,
        CEC_USER_CONTROL_CODE_F4_YELLOW = 0x74,
        CEC_USER_CONTROL_CODE_F5 = 0x75,
        CEC_USER_CONTROL_CODE_DATA = 0x76,
        // reserved: 0x77 ... 0xFF
        CEC_USER_CONTROL_CODE_AN_RETURN = 0x91, // return (Samsung)
        CEC_USER_CONTROL_CODE_AN_CHANNELS_LIST = 0x96, // channels list (Samsung)
        CEC_USER_CONTROL_CODE_MAX = 0x96,
        CEC_USER_CONTROL_CODE_UNKNOWN = 0xFF
    } usercontrol_code_t;

    typedef enum feature_all_device_type : uint8_t {
        ALL_DEVICE_TV = 0x80,
        ALL_DEVICE_ECORDER = 0x40,
        ALL_DEVICE_TUNER = 0x20,
        ALL_DEVICE_PLAYBACK = 0x10,
        ALL_DEVICE_AUDIOSYSTEM = 0x08,
        ALL_DEVICE_SWITCH = 0x04,
    } feature_all_device_t;

    typedef enum rc_profile_type : uint8_t {
        RC_PROFILE_TV_PROFILE_NONE = 0x00,
        RC_PROFILE_TV_PROFILE_1 = 0x02,
        RC_PROFILE_TV_PROFILE_2 = 0x06,
        RC_PROFILE_TV_PROFILE_3 = 0x0a,
        RC_PROFILE_TV_PROFILE_4 = 0x0e,
        RC_PROFILE_SRC_HAS_DEV_ROOT_MENU = 0x50,
        RC_PROFILE_SRC_HAS_DEV_SETUP_MENU = 0x48,
        RC_PROFILE_SRC_HAS_CONTENTS_MENU = 0x44,
        RC_PROFILE_SRC_HAS_MEDIA_TOP_MENU = 0x42,
        RC_PROFILE_SRC_HAS_MEDIA_CONTEXT_MENU = 0x41,
        RC_PROFILE_IS_EXTENDED = 0x80
    } rc_profile_t;

    typedef enum device_features_type : uint8_t {
        FEATURE_HAS_RECORD_TV_SCREEN = 0x40,
        FEATURE_HAS_SET_OSD_STRING = 0x20,
        FEATURE_HAS_DECK_CONTROL = 0x10,
        FEATURE_HAS_SET_AUDIO_RATE = 0x08,
        FEATURE_SINK_HAS_ARC_TX = 0x04,
        FEATURE_SOURCE_HAS_ARC_RX = 0x02,
        FEATURE_IS_EXTENDED = 0x80
    } device_features_t;

} // namespace CEC
} // namespace WPEFramework