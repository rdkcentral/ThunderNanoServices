/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file dsTypes.h
 *
 * @brief Device Settings HAL types.
 *
 * This API defines common types used by the Device Settings HAL.
 *
 * @par Document
 * Document reference.
 *
 * @par Open Issues (in no particular order)
 * -# None
 *
 * @par Assumptions
 * -# None
 *
 * @par Abbreviations
 * - BE:      Big-Endian.
 * - cb:      Callback function (suffix).
 * - DS:      Device Settings.
 * - FPD:     Front-Panel Display.
 * - HAL:     Hardware Abstraction Layer.
 * - LE:      Little-Endian.
 * - LS:      Least Significant.
 * - MBZ:     Must be zero.
 * - MS:      Most Significant.
 * - RDK:     Reference Design Kit.
 * - _t:      Type (suffix).
 *
 * @par Implementation Notes
 * -# None
 *
 */

/** 
 * @defgroup DSSETTINGS_HAL Device Settings HAL Types & Public API
 * HAL types and public API definitions for SoC/OEM components of the RDK.
 * @ingroup DSSETTINGS
 */

/**
 * @defgroup DSHAL_API Device Settings HAL Data Types
 * @ingroup DSSETTINGS_HAL
 *
 * The RDK Device Settings (DS) component is a cross-platform library for controlling
 * STB platform hardware configuration. The areas controlled by the DS component are:
 *  - Audio output ports (volume, mute, etc.),
 *  - Video ouptut ports (resolutions, aspect ratio, etc.),
 *  - Video device (zoom settings)
 *  - Display (aspect ratio, EDID data, etc.),
 *  - Front-panel indicators,
 *  - General platform (host) configuration,
 *
 * @note Each module in the Device Settings component has an initialization function and
 * a termination function. Clients of the HAL API are required to call the initialization
 * function first before using any other APIs from the module. If the initialization
 * function is not yet called, all HAL API invocations shall return ::dsERR_INVALID_STATE.
 * If an API is not supported on a platform, the implementation shall return
 * ::dsERR_OPERATION_NOT_SUPPORTED. Otherwise, the API shall return ::dsERR_NONE upon
 * success, or return the most suitable error code. Vendors implementing HAL APIs are
 * allowed to add to the dsError_t enumerations, but should only do so with discretion.
 *
 * @defgroup DSHAL_HOST Device Settings HAL Host Types
 * @ingroup DSHAL_API
 *
 * DeviceSettings HAL types and public API definitions that are part of the Device
 * Settings Host subsystem.
 *
 * @defgroup DSHAL_AUDIO Device Settings HAL Audio Types
 * @ingroup DSHAL_API
 *
 *  DeviceSettings HAL types and public API definitions that are part of the Device
 *  Settings Audio subsystem.
 *
 * @defgroup DSHAL_VIDEO Device Settings HAL Video Sub-Component
 * @ingroup DSHAL_API
 *
 * DeviceSettings HAL types and public API definitions that are part of the Device
 * Settings Video subsystem.  The video subsystem consists of two sub-components, the
 * Video Port subsystem and the Video Device subsystem.
 *
 * @defgroup DSHAL_VIDEOPORT Device Settings HAL VideoPort Types
 * @ingroup DSHAL_VIDEO
 *
 * DeviceSettings HAL types and public API definitions that are part of the Device
 * Settings Video Port subsystem.
 *
 * @defgroup DSHAL_VIDEODEVICE Device Settings HAL VideoDevice Types
 * @ingroup DSHAL_VIDEO
 *
 * DeviceSettings HAL types and public API definitions that are part of the Device
 * Settings Video Device subsystem.
 *
 * @defgroup DSHAL_DISPLAY Device Settings HAL Display Types
 * @ingroup DSHAL_API
 *
 * DeviceSettings HAL types and public API definitions that are part of the Device
 * Settings Display subsystem.
 *
 * @defgroup DSHAL_FPD Device Settings HAL Front Panel Display Types
 * @ingroup DSHAL_API
 *
 * DeviceSettings HAL types and public API definitions that are part of the Device
 * Settings Front Panel Display (FPD) subsystem.
 * 
 * @defgroup DSHAL_HDMI_IN Device Settings HAL HDMI Input Types
 * @ingroup DSHAL_API
 *
 * DeviceSettings HAL types and public API definitions that are part of the Device
 * Settings HDMI In subsystem.
 *
 * @defgroup DSHAL_VERSION Device Settings HAL Version Types
 * @ingroup DSHAL_API
 *
 * DeviceSettings HAL types and public API definitions that are part of the Device
 * Settings HDMI In subsystem.
 */

/**
* @defgroup devicesettings
* @{
* @defgroup hal
* @{
**/

#ifndef _DS_TYPES_H_
#define _DS_TYPES_H_

#include <stdint.h>
#include <sys/types.h>

#include <stdbool.h>

/** @addtogroup DSHAL_AUDIO Device Settings HAL Audio Types
 *  @{
 */

#define dsAUDIOPORT_TYPE_NUM_MAX 4 /**< Maximum number of audio output port types.    */
#define dsAudioPORT_NUM_MAX 4 /**< Maximum number of audio output ports.         */

/**
 * This enumeration defines the type of output audio port that are avaiable.
 */
typedef enum _dsAudioPortType_t {
    dsAUDIOPORT_TYPE_ID_LR, /**< RCA audio output.                  */
    dsAUDIOPORT_TYPE_HDMI, /**< HDMI audio output.                 */
    dsAUDIOPORT_TYPE_SPDIF, /**< S/PDIF audio output.               */
    dsAUDIOPORT_TYPE_MAX /**< Maximum index for audio port type. */
} dsAudioPortType_t;

/**
 * Audio output port type validation check.
 */
#define dsAudioType_isValid(t) (((t) >= dsAUDIOPORT_TYPE_ID_LR) && ((t) < dsAUDIOPORT_TYPE_MAX))

/**
 * This enumeration defines the type of audio encoding that can be used.
 */
typedef enum _dsAudioEncoding_t {
    dsAUDIO_ENC_NONE = 0, /**< No digital audio output.                         */
    dsAUDIO_ENC_DISPLAY, /**< Platform-selected digital audio encoding format. */
    dsAUDIO_ENC_PCM, /**< PCM digital audio encoding format.               */
    dsAUDIO_ENC_AC3, /**< AC-3 digital audio encoding format.               */
    dsAUDIO_ENC_EAC3, /**< DD+/E-AC-3 digital audio encoding format.               */
    dsAUDIO_ENC_MAX /**< Maximum index for audio encoding types.          */
} dsAudioEncoding_t;

/**
 * Audio encoding type validation check.
 */
#define dsAudioEncoding_isValid(t) (((t) >= dsAUDIO_ENC_NONE) && ((t) < dsAUDIO_ENC_MAX))

/**
 * This enumeration defines the type of audio compression that can be used.
 */
typedef enum _dsAudioCompression_t {
    dsAUDIO_CMP_NONE, /**< No audio compression.                      */
    dsAUDIO_CMP_LIGHT, /**< Light audio level compression.             */
    dsAUDIO_CMP_MEDIUM, /**< Medium audio level compression.            */
    dsAUDIO_CMP_HEAVY, /**< Heavy audio level compression.             */
    dsAUDIO_CMP_MAX /**< Maximum index for audio compression types. */
} dsAudioCompression_t;

/**
 * Audio compression type validation check.
 */
#define dsAudioCompression_isValid(t) (((t) >= dsAUDIO_CMP_NONE) && ((t) < dsAUDIO_CMP_MAX))

/**
 * This enumeration defines the type of audio stereo mode that can be used.
 */
typedef enum StereoMode {
    dsAUDIO_STEREO_UNKNOWN, /**< Unknown mode.                         */
    dsAUDIO_STEREO_MONO = 1, /**< Mono mode.                            */
    dsAUDIO_STEREO_STEREO, /**< Normal stereo mode (L+R).             */
    dsAUDIO_STEREO_SURROUND, /**< Surround mode.                        */
    dsAUDIO_STEREO_PASSTHRU, /**< Passthrough mode.                     */
    dsAUDIO_STEREO_MAX /**< Maximum index for audio stereo types. */
} dsAudioStereoMode_t;

/**
 * Audio stereo mode type validation check.
 */
#define dsAudioStereoMode_isValid(t) (((t) >= dsAUDIO_STEREO_UNKNOWN) && ((t) < dsAUDIO_STEREO_MAX))

/* End of DSHAL_AUDIO_TYPES doxygen group */
/**
 * @}
 */

/* This enumeration defines HDCP protocol version types */

typedef enum _dsHdcpProtocolVersion_t {
    dsHDCP_VERSION_1X = 0, /**< HDCP Protocol version 1.x */
    dsHDCP_VERSION_2X, /**< HDCP Protocol version 2.x */
    dsHDCP_VERSION_MAX /**< Maximum index for HDCP protocol. */
} dsHdcpProtocolVersion_t;

/* This enumeration defines all HDCP Authentication Status */

typedef enum _dsHdcpStatus_t {
    dsHDCP_STATUS_UNPOWERED = 0, /**< Connected Sink Device does not support HDCP */
    dsHDCP_STATUS_UNAUTHENTICATED, /**< HDCP Authentication Process is not initiated */
    dsHDCP_STATUS_AUTHENTICATED, /**< HDCP Authentication Process is initiated and Passed */
    dsHDCP_STATUS_AUTHENTICATIONFAILURE, /**< HDCP Authentication Failure or Link Integroty Failure */
    dsHDCP_STATUS_INPROGRESS, /**< HDCP Authentication in Progress */
    dsHDCP_STATUS_MAX /**< Maximum index for HDCP status. */
} dsHdcpStatus_t;

#define dsHdcpStatus_isValid(t) (((t) >= dsHDCP_STATUS_UNPOWERED) && ((t) < dsHDCP_STATUS_MAX))

/** @addtogroup DSHAL_VIDEOPORT_TYPES Device Settings HAL VideoPort Type Definitions
 *  @ingroup DSHAL_VIDEOPORT
 *  @{
 */

/**
 * This enumeration defines all of the standard type of Video ports that are available.
 */
typedef enum _dsVideoPortType_t {
    dsVIDEOPORT_TYPE_RF = 0, /**< RF modulator (channel 3/4) video output.             */
    dsVIDEOPORT_TYPE_BB, /**< Baseband (composite, RCA) video output.              */
    dsVIDEOPORT_TYPE_SVIDEO, /**< S-Video video output.                                */
    dsVIDEOPORT_TYPE_1394, /**< IEEE 1394 (Firewire) video output.                   */
    dsVIDEOPORT_TYPE_DVI, /**< DVI (Panel-Link, HDCP) video output.                 */
    dsVIDEOPORT_TYPE_COMPONENT, /**< Component video output.                              */
    dsVIDEOPORT_TYPE_HDMI, /**< HDMI video output.                                   */
    dsVIDEOPORT_TYPE_HDMI_INPUT, /**< HDMI video input.                                    */
    dsVIDEOPORT_TYPE_INTERNAL, /**< Internal (integrated/internal display) video output. */
    dsVIDEOPORT_TYPE_SCART, /**< SCART video output.                                  */
    dsVIDEOPORT_TYPE_MAX /**< Maximum index for video port types.                  */
} dsVideoPortType_t;

/**
 * Video ouptut port type validation check.
 */
#define dsVideoPortType_isValid(t) (((t) >= dsVIDEOPORT_TYPE_RF) && ((t) < dsVIDEOPORT_TYPE_MAX))

/**
 * This enumeration defines all of the standard video port resolutions that can be used.
 */
typedef enum _dsVideoResolution_t {
    dsVIDEO_PIXELRES_720x480, /**< 720x480 Resolution.                         */
    dsVIDEO_PIXELRES_720x576, /**< 720x576 Resolution.                         */
    dsVIDEO_PIXELRES_1280x720, /**< 1280x720 Resolution.                        */
    dsVIDEO_PIXELRES_1920x1080, /**< 1920x1080 Resolution.                       */
    dsVIDEO_PIXELRES_3840x2160, /**< 3840x2160 Resolution.                       */
    dsVIDEO_PIXELRES_4096x2160, /**< 3840x2160 Resolution.                       */

    dsVIDEO_PIXELRES_MAX /**< Maximum index for Video ouptut resolutions. */
} dsVideoResolution_t;

/**
 * This enumeration defines all of the standard TV supported resolution with interlace information.
 */
typedef enum _dsTVResolution_t {
    dsTV_RESOLUTION_480i = 0x0001, /**< 480i Resolution.                         */
    dsTV_RESOLUTION_480p = 0x0002, /**< 480p Resolution.                         */
    dsTV_RESOLUTION_576i = 0x0004, /**< 576p Resolution.                         */
    dsTV_RESOLUTION_576p = 0x0008, /**< 576p Resolution.                         */
    dsTV_RESOLUTION_720p = 0x0010, /**< 720p Resolution.                         */
    dsTV_RESOLUTION_1080i = 0x0020, /**< 1080i Resolution.                         */
    dsTV_RESOLUTION_1080p = 0x0040, /**< 1080p Resolution.                         */
    dsTV_RESOLUTION_2160p30 = 0x0080, /**< 2160p30 Resolution.                         */
    dsTV_RESOLUTION_2160p60 = 0x0100, /**< 2160p60 Resolution.                         */
} dsTVResolution_t;

/**
 * Video ouptut resolutions type validation check.
 */
#define dsVideoPortPixelResolution_isValid(t) (((t) >= dsVIDEO_PIXELRES_720x480) && ((t) < dsVIDEO_PIXELRES_MAX))

/**
 * This enumeration defines all of the standard frame rates at which video may be played out of the video port.
 */
typedef enum _dsVideoFrameRate_t {
    dsVIDEO_FRAMERATE_UNKNOWN, /**< Unknown frame rate.                       */
    dsVIDEO_FRAMERATE_24, /**< Played at 24 frames per second.           */
    dsVIDEO_FRAMERATE_25, /**< Played at 25 frames per second.           */
    dsVIDEO_FRAMERATE_30, /**< Played at 30 frames per second.           */
    dsVIDEO_FRAMERATE_60, /**< Played at 60 frames per second.           */
    dsVIDEO_FRAMERATE_23dot98, /**< Played at 23.98 frames per second.        */
    dsVIDEO_FRAMERATE_29dot97, /**< Played at 29.97 frames per second.        */
    dsVIDEO_FRAMERATE_50, /**< Played at 50 frames per second.           */
    dsVIDEO_FRAMERATE_59dot94, /**< Played at 59.94 frames per second.        */
    dsVIDEO_FRAMERATE_MAX /**< Maximum index for video frame rates.      */
} dsVideoFrameRate_t;

/**
 * Video output framerate validation check.
 */
#define dsVideoPortFrameRate_isValid(t) (((t) >= dsVIDEO_FRAMERATE_UNKNOWN) && ((t) < dsVIDEO_FRAMERATE_MAX))

/**
 * This enumeration defines all of the standard video port scan modes.
 */
typedef enum _dsVideoScanMode_t {
    dsVIDEO_SCANMODE_INTERLACED, /**< Interlaced video.                   */
    dsVIDEO_SCANMODE_PROGRESSIVE, /**< Progressive video.                  */
    dsVIDEO_SCANMODE_MAX /**< Maximum index for video scan modes. */
} dsVideoScanMode_t;

/**
 * Video scan mode type validation check.
 */
#define dsVideoPortScanMode_isValid(t) (((t) >= dsVIDEO_SCANMODE_INTERLACED) && ((t) < dsVIDEO_SCANMODE_MAX))

/**
 * This enumeration defines all of the standard video aspect ratios.
 */
typedef enum _dsVideoAspectRatio_t {
    dsVIDEO_ASPECT_RATIO_4x3, /**< 4:3 aspect ratio.                      */
    dsVIDEO_ASPECT_RATIO_16x9, /**< 16:9 aspect ratio.                     */
    dsVIDEO_ASPECT_RATIO_MAX /**< Maximum index for video aspect ratios. */
} dsVideoAspectRatio_t;

/**
 * Video aspect ratio type validation check.
 */
#define dsVideoPortAspectRatio_isValid(t) (((t) >= dsVIDEO_ASPECT_RATIO_4x3) && ((t) < dsVIDEO_ASPECT_RATIO_MAX))

/**
 * This enumeration defines all of the standard video Stereo Scopic modes.
 */
typedef enum _dsVideoStereoScopicMode_t {
    dsVIDEO_SSMODE_UNKNOWN = 0, /**< Unknown mode.                                */
    dsVIDEO_SSMODE_2D, /**< 2D mode.                                     */
    dsVIDEO_SSMODE_3D_SIDE_BY_SIDE, /**< 3D side by side (L/R) stereo mode.           */
    dsVIDEO_SSMODE_3D_TOP_AND_BOTTOM, /**< 3D top & bottom stereo mode.                 */
    dsVIDEO_SSMODE_MAX /**< Maximum index for video stereoscopic modes.  */
} dsVideoStereoScopicMode_t;

/**
 * Video Stereo Scopic modes type validation check.
 */
#define dsVideoPortStereoScopicMode_isValid(t) (((t) >= dsVIDEO_SSMODE_UNKNOWN) && ((t) < dsVIDEO_SSMODE_MAX))

/**
 * This enumeration defines all of the standard screen zoom (format conversion) modes.
 */
typedef enum _dsVideoZoom_t {
    dsVIDEO_ZOOM_UNKNOWN = -1, /**< Unknown mode.                                                                                   */
    dsVIDEO_ZOOM_NONE = 0, /**< Decoder format conversion is inactive.                                                          */
    dsVIDEO_ZOOM_FULL, /**< Full screen (16:9 video is streched to fit 4:3 frame).                                          */
    dsVIDEO_ZOOM_LB_16_9, /**< 16:9 Letterbox (16:9 video is placed in a 4:3 frame with a full letterbox effect).              */
    dsVIDEO_ZOOM_LB_14_9, /**< 14:9 Letterbox (16:9 video is transferred into a 4:3 frame with 14:9 letterbox effect).         */
    dsVIDEO_ZOOM_CCO, /**< Center Cut-out (16:9 video is cut to fill 4:3 frame with its center part).                      */
    dsVIDEO_ZOOM_PAN_SCAN, /**< Pan & Scan (16:9 is cut and panned to fille 4:3 frame, following MPEG pan & scan vectors).      */
    dsVIDEO_ZOOM_LB_2_21_1_ON_4_3, /**< 2.21:1 Letterbox on 4:3 (video is transferred into 4:3 frame with a 2.21:1 letterbox effect).   */
    dsVIDEO_ZOOM_LB_2_21_1_ON_16_9, /**< 2.21:1 Letterbox on 16:9 (video is transferred into 16:9 frame with a 2.21:1 letterbox effect). */
    dsVIDEO_ZOOM_PLATFORM, /**< Control over the decoder format conversions is managed by the platform.                         */
    dsVIDEO_ZOOM_16_9_ZOOM, /**< 16:9 Zoom (4:3 video is zoomed to fill 16:9 frame).                                             */
    dsVIDEO_ZOOM_PILLARBOX_4_3, /**< Pillarbox 4:3 (4:3 video is placed in a 16:9 frame with a pillarbox effect)                     */
    dsVIDEO_ZOOM_WIDE_4_3, /**< Wide 4:3 (4:3 video is stretched to fill 16:9 frame).                                           */
    dsVIDEO_ZOOM_MAX /**< Maximum index for screen zoom modes.                                                            */
} dsVideoZoom_t;

/**
 * Video screen zoom validation check.
 */
#define dsVideoPortDFC_isValid(t) (((t) >= dsVIDEO_ZOOM_NONE) && ((t) < dsVIDEO_ZOOM_MAX))

/* End of DSHAL_VIDEOPORT_TYPES doxygen group. */
/**
 * @}
 */

/** @addtogroup DSHAL_VIDEODEVICE_TYPES Device Settings HAL VideoDevice Type Definitions
 *  @ingroup DSHAL_VIDEODEVICE
 *  @{
 */

/**
 * @brief Structure that defines video device configuration for the output.
 */
typedef struct _dsVideoConfig_t {
    size_t numSupportedDFCs; /**< Number of zoom modes supported. */
    const dsVideoZoom_t* supportedDFCs; /**< List of zoom modes supported.   */
    dsVideoZoom_t defaultDFC; /**< The default zoom mode.          */
} dsVideoConfig_t;
/* End of DSHAL_VIDEODEVICE_TYPES doxygen group. */
/**
 * @}
 */

/**
 * @ingroup DSHAL_AUDIO_TYPES
 * @brief Structure that defines audio output device configuration.
 */
typedef struct _dsAudioTypeConfig_t {
    int32_t typeId; /**< The audio output type (::dsAudioPortType_t).       */
    const char* name; /**< Name of the audio output device (e.g. "HDMI").     */
    size_t numSupportedCompressions; /**< Number of supported audio compression methods.     */
    const dsAudioCompression_t* compressions; /**< List of audio compression methods supported.       */
    size_t numSupportedEncodings; /**< Number of supported audio encodings.               */
    const dsAudioEncoding_t* encodings; /**< List of audio encodings supported.                 */
    size_t numSupportedStereoModes; /**< Number of supported stereo modes.                  */
    const dsAudioStereoMode_t* stereoModes; /**< List of stereo modes supported.                    */
} dsAudioTypeConfig_t;

/**
 * @ingroup DSHAL_VIDEOPORT_TYPES
 * @brief Structure that defines video port resolution settings of output video device.
 */
typedef struct _dsVideoPortResolution_t {
    char name[32]; /**< Name the resolution (e.g. 480i, 480p, 1080p24).   */
    dsVideoResolution_t pixelResolution; /**< The resolution associated with the name.                 */
    dsVideoAspectRatio_t aspectRatio; /**< The associated aspect ratio.                             */
    dsVideoStereoScopicMode_t stereoScopicMode; /**< The associated stereoscopic mode.                        */
    dsVideoFrameRate_t frameRate; /**< The associated frame rate.                               */
    bool interlaced; /**< The associated scan mode(@a true if interlaced, @a false if progressive). */
} dsVideoPortResolution_t;

/**
 * @ingroup DSHAL_AUDIO_TYPES
 * @brief Structure that defines the audio port type and associated ID.
 */
typedef struct _dsAudioPortId_t {
    dsAudioPortType_t type; /**< Audio port type. */
    int32_t index; /**< Port ID/number.  */
} dsAudioPortId_t;

/**
 * @ingroup DSHAL_VIDEOPORT_TYPES
 * @brief Structure that defines port id associated with video port.
 */
typedef struct _dsVideoPortPortId_t {
    dsVideoPortType_t type; /**< Video port type. */
    int32_t index; /**< Port ID/number. */
} dsVideoPortPortId_t;

/**
 * @ingroup DSHAL_VIDEOPORT_TYPES
 * @brief Structure that defines the video output port configuration.
 */
typedef struct _dsVideoPortTypeConfig_t {
    dsVideoPortType_t typeId; /**< The video output type.               */
    const char* name; /**< Name of the video output port.       */
    bool dtcpSupported; /**< Is DTCP supported?                   */
    bool hdcpSupported; /**< Is HDCP supported?                   */
    int32_t restrictedResollution; /**< Any restricted resolution; -1 if no. */
    size_t numSupportedResolutions; /**< Number of supported resolutions.     */
    dsVideoPortResolution_t* supportedResolutions; /**< List of supported resolutions.       */
} dsVideoPortTypeConfig_t;

//#define HDCP_KEY_MAX_SIZE  (1280)
#define HDCP_KEY_MAX_SIZE (4 * 1024)

/**
 * Enumeration defines all of the standard HDR types.
 * Each bit of uint32_t represent a standard. 
 * If a device supports multiple standards, the capability is the bitwise OR
* of the standards.
 */
typedef enum _dsHDRStandard_t {
    dsHDRSTANDARD_NONE = 0x0,
    dsHDRSTANDARD_HDR10 = 0x01,
    dsHDRSTANDARD_HLG = 0x02,
    dsHDRSTANDARD_DolbyVision = 0x04,
    dsHDRSTANDARD_TechnicolorPrime = 0x08,
} dsHDRStandard_t;

/**
 * Enumeration defines surround mode.
 * Each bit of uint32_t represent supported surround mode. 
 */
typedef enum _dsSURROUNDMode_t {
    dsSURROUNDMODE_NONE = 0x0,
    dsSURROUNDMODE_DD = 0x1,
    dsSURROUNDMODE_DDPLUS = 0x2,
} dsSURROUNDMode_t;

/**
 * Enumeration defines MS12 feature.
 */
typedef enum _dsMS12FEATURE_t {
    dsMS12FEATURE_DAPV2 = 0x0,
    dsMS12FEATURE_DE = 0x1,
    dsMS12FEATURE_MAX = 0x2,
} dsMS12FEATURE_t;
/**
 * @ingroup DSHAL_AUDIO_TYPES
 * @brief Structure that defines audio port configuration.
 */
typedef struct _dsAudioPortConfig_t {
    dsAudioPortId_t id; /**< Port ID.               */
    const dsVideoPortPortId_t* connectedVOPs; /**< Connected video port.  */
} dsAudioPortConfig_t;

/**
 * @ingroup DSHAL_VIDEOPORT_TYPES
 * @brief Structure that defines video port configuration settings.
 */
typedef struct _dsVideoPortPortConfig_t {
    dsVideoPortPortId_t id; /**< Port ID.               */
    dsAudioPortId_t connectedAOP; /**< Connected audio port.  */
    const char* defaultResolution; /**< Default resolution's name. */
} dsVideoPortPortConfig_t;

/** @addtogroup DSHAL_FPD_TYPES Device Settings HAL Front Panel Display (FPD) Type Definitions
 *  @ingroup DSHAL_FPD
 *  @{
 */

/*
 * dsFPDColor_t is an 4-byte integer that is composed of RGB32 value in the
 * following pattern:
 *
 * FPD_COLOR = (((R8)<<16) | ((G8)<< 8) | ((B8) << 0))
 *
 * The Most Significant 8 bit is reserved.
 *
 * Please use the following macros to manipulate FPD_COLOR.
 */
typedef uint32_t dsFPDColor_t;
#define dsFPDColor_Make(R8, G8, B8) (((R8) << 16) | ((G8) << 8) | ((B8)))
#define dsFPDColor_R(RGB32) (((RGB32) >> 16) & 0xFF)
#define dsFPDColor_G(RGB32) (((RGB32) >> 8) & 0xFF)
#define dsFPDColor_B(RGB32) (((RGB32)) & 0xFF)

/* Define a set of common colors, for backward compatibility */
#define dsFPD_COLOR_BLUE dsFPDColor_Make(0, 0, 0xFF) /**<  Blue color LED.                   */
#define dsFPD_COLOR_GREEN dsFPDColor_Make(0, 0xFF, 0) /**<  Green color LED.                  */
#define dsFPD_COLOR_RED dsFPDColor_Make(0xFF, 0, 0x0) /**<  Red color LED.                    */
#define dsFPD_COLOR_YELLOW dsFPDColor_Make(0xFF, 0xFF, 0xE0) /**<  Yellow color LED.                 */
#define dsFPD_COLOR_ORANGE dsFPDColor_Make(0xFF, 0x8C, 0x00) /**<  Orange color LED.                 */
#define dsFPD_COLOR_WHITE dsFPDColor_Make(0xFF, 0xFF, 0xFF) /**<  White color LED.                 */
#define dsFPD_COLOR_MAX 6 /**<  Maximum idx of predefined colors. */

/**
 * Front panel LED colors validation check.
 */
#define dsFPDColor_isValid(t) (((t)&0xFF000000) == 0)

/*
 * dsFPDIndicator_t is an ID number that  uniquely identifies a hardware
 * LED entity on the front panel.  Each MFR implementation is free
 * to assign any number to its LEDs. The mapping of LED ID to its
 * display name (such as "Record LED") is done outside MFR library.
 */
typedef int32_t dsFPDIndicator_t;
/**
 * This enumeration pre-defines common front panel indicators.
 * Implementation may not have to use these enumerators.
 */
#define dsFPD_INDICATOR_MESSAGE 0 /**< Message/Mail LED.           */
#define dsFPD_INDICATOR_POWER 1 /**< Power LED.                  */
#define dsFPD_INDICATOR_RECORD 2 /**< Record LED.                 */
#define dsFPD_INDICATOR_REMOTE 3 /**< Remote LED.                 */
#define dsFPD_INDICATOR_RFBYPASS 4 /**< RF Bypass LED.              */
#define dsFPD_INDICATOR_MAX 5
/**
 * Front panel LED indicator type validation check.
 */
#define dsFPDIndicator_isValid(t) (true)

/**
 * This enumeration defines the FPD text display types.
 */
typedef enum __dsFPDTextDisplay_t {
    dsFPD_TEXTDISP_TEXT, /**< 7-segment LED display.                    */
    dsFPD_TEXTDISP_MAX /**< Maximum index for FPD text display types. */
} dsFPDTextDisplay_t;

/**
 * FPD text display type validation check.
 */
#define dsFPDTextDisplay_isValid(t) (((t) >= dsFPD_TEXTDISP_TEXT) && ((t) < dsFPD_TEXTDISP_MAX))

/**
 * This enumeration defines the front panel display state.
 */
typedef enum __dsFPDState_t {
    dsFPD_STATE_OFF = 0, /**< FPD State Disable. */
    dsFPD_STATE_ON, /**< FPD State Enable. */
} dsFPDState_t;

/**
 * This enumeration defines the time formats to be used in the Front panel text display
 */
typedef enum __dsFPDTimeFormat_t {
    dsFPD_TIME_12_HOUR, /**< 12 hour time format. */
    dsFPD_TIME_24_HOUR, /**< 24 hour time format. */
    dsFPD_TIME_STRING /**< Text string.         */
} dsFPDTimeFormat_t;

/**
 * Maximum Value of FPD Led Brightness
 */
#define dsFPD_BRIGHTNESS_MAX 100 /**< Maximum Value of FPD LED brightness. */

/**
 * Type definition for the brightness setting of a front panel indicator LED.
 *
 */
typedef uint32_t dsFPDBrightness_t;

/* End of DSHAL_FPD_TYPES doxygen group */
/**
 * @}
 */

/** @addtogroup DSHAL_DISPLAY_TYPES Device Settings HAL Display Type Definitions
 *  @ingroup DSHAL_DISPLAY
 *  @{
 */

/**
 * Maximum Value  video modes are described in CEA specifictaion
 */

#define dsEEDID_MAX_VIDEO_CODE 64 /**< Maximum Value EEDID Video COde . */

#define dsEEDID_MAX_MON_NAME_LENGTH 14 /**< Maximum Length of Connected Display Monitor Name . */

/**
 * @ingroup DSHAL_DISPLAY_TYPES
 * @brief Defines the structure that is used to get the EDID information of the video display.
 *
 */
typedef struct _dsDisplayEDID_t {
    int32_t productCode; /**< Product code of the display device.        */
    int32_t serialNumber; /**< Serial number of the display device.       */
    int32_t manufactureYear; /**< Year of manufacture of the display device. */
    int32_t manufactureWeek; /**< Week of manufacture of the display device. */
    bool hdmiDeviceType; /**< Device type (@ true if HDMI, @a false if DVI). */
    bool isRepeater; /**<  Receiver is a repeater  (@ true if Repeater, @a false if connected Receiver is not a repeater). */
    uint8_t physicalAddressA; /**<  Physical Address for HDMI node A  */
    uint8_t physicalAddressB; /**<  Physical Address for HDMI node B  */
    uint8_t physicalAddressC; /**<  Physical Address for HDMI node C -*/
    uint8_t physicalAddressD; /**<  Physical Address for HDMI node D */
    int32_t numOfSupportedResolution; /**<  Number of Supported Resolution */
    dsVideoPortResolution_t suppResolutionList[dsEEDID_MAX_VIDEO_CODE * dsVIDEO_SSMODE_MAX]; /**<  EDID Supported Resoultion List */
    char monitorName[dsEEDID_MAX_MON_NAME_LENGTH]; /**<  Connected Display Monitor Name */
} dsDisplayEDID_t;

/**
 * This enumeration defines the type of sleep modes that can be used.
 */
typedef enum _dsSleepMode_t {
    dsHOST_SLEEP_MODE_LIGHT, /**< Light sleep mode.                                */
    dsHOST_SLEEP_MODE_DEEP, /**< Deep sleep mode.                                 */
    dsHOST_SLEEP_MODE_MAX, /**< Maximum index for sleep modes                    */
} dsSleepMode_t;

typedef enum _dsPowerState_t {
    dsPOWER_ON = 1, /**< Host Power-on state   */
    dsPOWER_STANDBY, /**< Host Standby state.   */
    dsPOWER_OFF, /**< Host Power-off state. */
} dsPowerState_t;

/**
 * Sleep mode validation check.
 */
#define dsSleepMode_isValid(t) (((t) >= dsHOST_SLEEP_MODE_LIGHT) && ((t) < dsHOST_SLEEP_MODE_MAX))

/* End of DSHAL_DISPLAY_TYPES doxygen group */
/**
 * @}
 */

/** @addtogroup DSHAL_HDMI_IN_TYPES Device Settings HAL HDMI IN Type Definitions
 *  @ingroup DSHAL_HDMI_IN
 *  @{
 */
typedef enum _dsHdmiInPort_t {
    dsHDMI_IN_PORT_NONE = -1,
    dsHDMI_IN_PORT_0,
    dsHDMI_IN_PORT_1,
    dsHDMI_IN_PORT_MAX
} dsHdmiInPort_t;

typedef struct _dsHdmiInStatus_t {
    bool isPresented; /**< Boolean flag indicating Hdmi Input is enabled for presentation by client */
    bool isPortConnected[dsHDMI_IN_PORT_MAX]; /**< Boolean flag indicating Hdmi source connected to this Hdmi Input port */
    dsHdmiInPort_t activePort; /**< Hdmi Input Port selected as the currently active port (to the set-top)
                                                              - note that only one HDMI Input port can be active at a time */
} dsHdmiInStatus_t;

/* End of DSHAL_HDMI_IN_TYPES doxygen group */
/**
 * @}
 */

/* List of video compression formats that may be supported by the decoder. */
typedef enum {
    dsVIDEO_CODEC_MPEGHPART2 = (0x01 << 0), //Also known HEVC, H.265
    dsVIDEO_CODEC_MPEG4PART10 = (0x01 << 1), //Also known as H.264, MPEG4 AVC
    dsVIDEO_CODEC_MPEG2 = (0x01 << 2),
} dsVideoCodingFormat_t;

/* HEVC version 1 profiles are listed. More may be added to it as the support becomes available.*/
typedef enum {
    dsVIDEO_CODEC_HEVC_PROFILE_MAIN = (0x01 << 0),
    dsVIDEO_CODEC_HEVC_PROFILE_MAIN10 = (0x01 << 1),
    dsVIDEO_CODEC_HEVC_PROFILE_MAINSTILLPICTURE = (0x01 << 2),
} dsVideoCodecHevcProfiles_t;

typedef struct
{
    dsVideoCodecHevcProfiles_t profile;
    float level;
} dsVideoCodecProfileSupport_t;

typedef struct
{
    unsigned int num_entries;
    dsVideoCodecProfileSupport_t entries[10];
} dsVideoCodecInfo_t;

/** @addtogroup DSHAL_VERSION_TYPES Device Settings HAL VERSION Definitions
 *  @ingroup DSHAL_VERSION
 *  @{
 */

#define dsHAL_APIVER(major, minor) (uint32_t)((major << 16) | (minor & 0xFFFF)) /**< Return 4 Bytes version value */

#define dsHAL_APIVER_MAJOR(x) ((x) >> 16) & (0x7FFF) /**< Return the two Most Significant Byte value */

#define dsHAL_APIVER_MINOR(x) (x & 0xFFFF) /**< Return the two Least Significant Byte value */

/* End of DSHAL_VERSION_TYPES  doxygen group */
/**
 * @}
 */
#endif

/** @} */
/** @} */
