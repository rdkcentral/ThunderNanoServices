/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#include "NexusServer.h"

#include <nexus_config.h>
#include <nexus_platform.h>
#include <nexus_types.h>
#include <nxclient.h>
#include <nexus_display_vbi.h>
#if NEXUS_HAS_VIDEO_DECODER
#include <nexus_video_decoder.h>
#endif
#include <nexus_display.h>
#include <sys/stat.h>

BDBG_MODULE(NexusServer);

namespace WPEFramework {

namespace Broadcom {
    /* virtual */ string Platform::Client::Name() const
    {
        return (::std::string(Id()));
    }

    /* virtual */ void Platform::Client::Opacity(const uint32_t value)
    {
        ASSERT(_client != nullptr);

        TRACE(Trace::Information, (_T("Alpha client %s to %d."), Name().c_str(), value));
        nxserverlib_set_server_alpha(_client, value);
    }
    uint32_t Platform::Client::Geometry(const Exchange::IComposition::Rectangle& rectangle) /* override */
    {
        ASSERT(_client != nullptr);
        _rectangle = rectangle;

        return (Core::ERROR_UNAVAILABLE);
    }
    Exchange::IComposition::Rectangle Platform::Client::Geometry() const /* override */
    {
        ASSERT(_client != nullptr);

        return (_rectangle);
    }
    uint32_t Platform::Client::ZOrder(const uint16_t index) /* override */
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _layer = index;

        ASSERT(_client != nullptr);

        if (_layer == 0) {
            result = Core::ERROR_NONE;

            /* the definition of "focus" is variable. this is one impl. */
            NxClient_ConnectList list;
            struct nxclient_status status;

            nxclient_get_status(_client, &status);
            NxClient_P_Config_GetConnectList(_client, status.handle, &list);

            /* only refresh first connect */
            if (list.connectId[0]) {
                NxClient_P_RefreshConnect(_client, list.connectId[0]);
            }
            nxserver_p_focus_input_client(_client);
            nxserver_p_focus_surface_client(_client);
        }
        return (result);
    }

    uint32_t Platform::Client::ZOrder() const {
        return _layer;
    }

    class Config : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        enum svptype {
            NONE = nxserverlib_svp_type_none,
            VIDEO = nxserverlib_svp_type_cdb, /* Force CRR for video CDB */
            ALL = nxserverlib_svp_type_cdb_urr /* Force CRR for video CDB + default to all secure only buffers */
        };

        enum hdcplevel {
            HDCP_NONE = NxClient_HdcpLevel_eNone, /* No HDCP requested by this client. */
            HDCP_OPTIONAL = NxClient_HdcpLevel_eOptional, /* Enable HDCP but do not mute video, even on authentication failure. */
            HDCP_MANDATORY = NxClient_HdcpLevel_eMandatory /* Enable HDCP and mute video until authentication success. */
        };

        enum hdcpversion {
            AUTO = NxClient_HdcpVersion_eAuto, /* Always authenticate using the highest version supported by HDMI receiver (Content Stream Type 0) */
            HDCP1X = NxClient_HdcpVersion_eHdcp1x, /* Always authenticate using HDCP 1.x mode (regardless of HDMI Receiver capabilities) */
            HDCP22 = NxClient_HdcpVersion_eHdcp22 /* Always authenticate using HDCP 2.2 mode, Content Stream Type 1 */
        };

        enum loudnessmode {
            LM_NONE = NEXUS_AudioLoudnessEquivalenceMode_eNone, /* Default, no loudness equivalence. */
            LM_ATSC_A85 = NEXUS_AudioLoudnessEquivalenceMode_eAtscA85, /* ATSC A/85. */
            LM_EBU_R128 = NEXUS_AudioLoudnessEquivalenceMode_eEbuR128 /* EBU-R128. */
        };

        class MemoryInfo : public Core::JSON::Container {
        private:
            MemoryInfo(const MemoryInfo&) = delete;
            MemoryInfo& operator=(const MemoryInfo&) = delete;

        public:
            MemoryInfo()
                : GFX()
                , GFX2()
                , Restricted()
                , Main()
                , Export()
                , Client()
                , SecureGFX()
            {
                Add(_T("gfx"), &GFX);
                Add(_T("gfx2"), &GFX2);
                Add(_T("restricted"), &Restricted);
                Add(_T("main"), &Main);
                Add(_T("export"), &Export);
                Add(_T("client"), &Client);
                Add(_T("secureGFX"), &SecureGFX);
            }
            ~MemoryInfo()
            {
            }

        public:
            Core::JSON::DecUInt16 GFX;
            Core::JSON::DecUInt16 GFX2;
            Core::JSON::DecUInt16 Restricted;
            Core::JSON::DecUInt16 Main;
            Core::JSON::DecUInt16 Export;
            Core::JSON::DecUInt16 Client;
            Core::JSON::DecUInt16 SecureGFX;
        };

    public:
        Config()
            : Core::JSON::Container()
            , IRMode(NEXUS_IrInputMode_eCirNec)
            , Authentication(true)
            , BoxMode(~0)
            , SagePath()
            , PAKPath()
            , DRMPath()
            , SVPType(NONE)
            , Memory()
            , HDCPLevel(HDCP_NONE)
            , HDCPVersion(AUTO)
            , HDCP1xBinFile()
            , HDCP2xBinFile()
            , LoudnessMode(LM_NONE)
        {
            Add(_T("irmode"), &IRMode);
            Add(_T("authentication"), &Authentication);
            Add(_T("boxmode"), &BoxMode);
            Add(_T("sagepath"), &SagePath);
            Add(_T("pakpath"), &PAKPath);
            Add(_T("drmpath"), &DRMPath);
            Add(_T("svp"), &SVPType);
            Add(_T("memory"), &Memory);
            Add(_T("framebufferwidth"), &FrameBufferWidth);
            Add(_T("framebufferheight"), &FrameBufferHeight);
            Add(_T("hdcplevel"), &HDCPLevel);
            Add(_T("hdcpversion"), &HDCPVersion);
            Add(_T("hdcp1xbinfile"), &HDCP1xBinFile);
            Add(_T("hdcp2xbinfile"), &HDCP2xBinFile);
            Add(_T("loudnessmode"), &LoudnessMode);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::EnumType<NEXUS_IrInputMode> IRMode;
        Core::JSON::Boolean Authentication;
        Core::JSON::DecUInt8 BoxMode;
        Core::JSON::String SagePath;
        Core::JSON::String PAKPath;
        Core::JSON::String DRMPath;
        Core::JSON::EnumType<svptype> SVPType;
        MemoryInfo Memory;
        Core::JSON::DecUInt16 FrameBufferWidth;
        Core::JSON::DecUInt16 FrameBufferHeight;
        Core::JSON::EnumType<hdcplevel> HDCPLevel;
        Core::JSON::EnumType<hdcpversion> HDCPVersion;
        Core::JSON::String HDCP1xBinFile;
        Core::JSON::String HDCP2xBinFile;
        Core::JSON::EnumType<loudnessmode> LoudnessMode;
    };

    /* -------------------------------------------------------------------------------------------------------------
     * This is a singleton. Declare all C accessors to this object here
     * ------------------------------------------------------------------------------------------------------------- */
    /* static */ Platform* Platform::_implementation = nullptr;

    static int find_unused_heap(const NEXUS_PlatformSettings& platformSettings)
    {
        for (int i = NEXUS_MAX_HEAPS - 1; i < NEXUS_MAX_HEAPS; i--) {
            if (!platformSettings.heap[i].size && !platformSettings.heap[i].heapType)
                return i;
        }
        return -1;
    }

    /* static */ void Platform::CloseDown()
    {
        // Seems we are destructed.....If we still have a pointer to the implementation, Kill it..
        if (_implementation != nullptr) {
            delete _implementation;
            _implementation = nullptr;
        }
    }

    template <typename... T>
    /* static */ int Platform::ClientConnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings,
        NEXUS_ClientSettings* pClientSettings, T... Targs)
    {
        BSTD_UNUSED(pClientSettings);

        if (_implementation != nullptr) {

            /* server app has opportunity to reject client altogether, or modify pClientSettings */
            _implementation->Add(client, pJoinSettings);
        }
        return (0);
    }

    /* static */ void Platform::ClientDisconnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings)
    {
        BSTD_UNUSED(pJoinSettings);

        if (_implementation != nullptr) {

            _implementation->Remove(pJoinSettings->name);
        }
    }

    Platform::~Platform()
    {
        _state = DEINITIALIZING;
        nxserver_ipc_uninit();
        nxserverlib_uninit(_instance);
        NEXUS_Platform_Uninit();
        BKNI_DestroyMutex(_lock);
        _implementation = nullptr;
    }

    Platform::Platform(IStateChange* stateChanges, IClient* clientChanges, const std::string& configuration, const NEXUS_VideoFormat& format)
        : _lock()
        , _instance()
        , _serverSettings()
        , _platformSettings()
        , _platformCapabilities()
        , _state(UNITIALIZED)
        , _clientHandler(clientChanges)
        , _stateHandler(stateChanges)
    {
        ASSERT(_implementation == nullptr);
        ASSERT(_instance == nullptr);

        // Strange someone already started a NXServer.
        if (_instance == nullptr) {

            _implementation = this;

            NEXUS_Error rc = NEXUS_SUCCESS;

            // Register an @Exit, in case we are killed, with an incorrect ref count !!
            if (atexit(CloseDown) != 0) {
                TRACE(Trace::Information, (("Could not register @exit handler. Error: %d."), errno));
                exit(EXIT_FAILURE);
            }

            TRACE_L1("Start Nexus server...%d\n", __LINE__);

            Config config;
            config.FromString(configuration);

            if ((config.SagePath.IsSet() == true) && (config.SagePath.Value().empty() == false)) {
                ::setenv("SAGEBIN_PATH", config.SagePath.Value().c_str(), 1);
            }

            if ((config.PAKPath.IsSet() == true) && (config.PAKPath.Value().empty() == false)) {
                ::setenv("PAKBIN_PATH", config.PAKPath.Value().c_str(), 1);
            }

            if ((config.DRMPath.IsSet() == true) && (config.DRMPath.Value().empty() == false)) {
                ::setenv("DRMBIN_PATH", config.DRMPath.Value().c_str(), 1);
            }

            if (config.BoxMode.IsSet()) {
                // Set box mode using env var.
                char stringNumber[10];
                snprintf(stringNumber, sizeof(stringNumber), "%d", config.BoxMode.Value());
                ::setenv("B_REFSW_BOXMODE", stringNumber, 1);
                TRACE_L1("Set BoxMode to %d\n", config.BoxMode.Value());
            }

            nxserver_get_default_settings(&(_serverSettings));
            NEXUS_Platform_GetDefaultSettings(&(_platformSettings));
            NEXUS_GetDefaultMemoryConfigurationSettings(&(_serverSettings.memConfigSettings));
            NEXUS_GetPlatformCapabilities(&(_platformCapabilities));

            if (config.Memory.IsSet()) {
                const Config::MemoryInfo& memory(config.Memory);

                if (memory.GFX.IsSet()) {
                    int index = nxserver_heap_by_type(&_platformSettings, NEXUS_HEAP_TYPE_GRAPHICS);
                    if (index != -1) {
                        TRACE_L1("Setting gfx heap[%d] to %dMB", index, memory.GFX.Value());
                        _platformSettings.heap[index].size = (memory.GFX.Value() * 1024 * 1024);
                    }
                }
                if (memory.GFX2.IsSet()) {
                    int index = nxserver_heap_by_type(&_platformSettings, NEXUS_HEAP_TYPE_SECONDARY_GRAPHICS);
                    if (index != -1) {
                        TRACE_L1("Setting gfx2 heap[%d] to %dMB", index, memory.GFX2.Value());
                        _platformSettings.heap[index].size = (memory.GFX2.Value() * 1024 * 1024);
                    }
                }
                if (memory.Restricted.IsSet()) {
                    int index = nxserver_heap_by_type(&_platformSettings, NEXUS_HEAP_TYPE_COMPRESSED_RESTRICTED_REGION);
                    if (index != -1) {
                        TRACE_L1("Setting secure video heap[%d] to %dMB", index, memory.Restricted.Value());
                        _platformSettings.heap[index].size = (memory.Restricted.Value() * 1024 * 1024);
                    }
                }
                if (memory.Main.IsSet()) {
                    int index = nxserver_heap_by_type(&_platformSettings, NEXUS_HEAP_TYPE_MAIN);
                    if (index != -1) {
                        TRACE_L1("Setting main heap[%d] to %dMB", index, memory.Main.Value());
                        _platformSettings.heap[index].size = (memory.Main.Value() * 1024 * 1024);
                    }
                }
                if (memory.Export.IsSet()) {
                    int index = nxserver_heap_by_type(&_platformSettings, NEXUS_HEAP_TYPE_EXPORT_REGION);
                    if (index != -1) {
                        TRACE_L1("Setting export heap[%d] to %dMB", index, memory.Export.Value());
                        _platformSettings.heap[index].size = (memory.Export.Value() * 1024 * 1024);
                    }
                }
                if (memory.Client.IsSet()) {
                    /* create a dedicated heap for the client */
                    int mainIndex = nxserver_heap_by_type(&_platformSettings, NEXUS_HEAP_TYPE_GRAPHICS);
                    if (mainIndex != -1) {
                        int unused_heap = find_unused_heap(_platformSettings);
                        if (unused_heap != -1) {
                            TRACE_L1("Setting client heap[%d] to %dMB", unused_heap, memory.Client.Value());
                            _serverSettings.heaps.clientFullHeap = unused_heap;
                            _platformSettings.heap[_serverSettings.heaps.clientFullHeap].size = (memory.Client.Value() * 1024 * 1024);
                            _platformSettings.heap[_serverSettings.heaps.clientFullHeap].memcIndex = _platformSettings.heap[mainIndex].memcIndex;
                            _platformSettings.heap[_serverSettings.heaps.clientFullHeap].subIndex = _platformSettings.heap[mainIndex].subIndex;
                            _platformSettings.heap[_serverSettings.heaps.clientFullHeap].guardBanding = false; /* corruptions shouldn't cause server failures */
                            _platformSettings.heap[_serverSettings.heaps.clientFullHeap].alignment = _platformSettings.heap[mainIndex].alignment;
                            _platformSettings.heap[_serverSettings.heaps.clientFullHeap].memoryType = NEXUS_MemoryType_eFull; /* requires for packet blit and playpump */
#if NEXUS_HAS_SAGE
                            /* sage must be told which heap the client's will be using */
                            _platformSettings.sageModuleSettings.clientHeapIndex = _serverSettings.heaps.clientFullHeap;
#endif
                        }
                    }
                }
                if (memory.SecureGFX.IsSet()) {
                    NEXUS_PlatformConfigCapabilities cap;
                    NEXUS_GetPlatformConfigCapabilities(&_platformSettings, &(_serverSettings.memConfigSettings), &cap);
                    if (cap.secureGraphics[0].valid) {
                        TRACE_L1("creating securegfx heap[%d] on MEMC%u for %dMB", cap.secureGraphics[0].heapIndex, cap.secureGraphics[0].memcIndex, memory.SecureGFX.Value());
                        _platformSettings.heap[cap.secureGraphics[0].heapIndex].size = (memory.SecureGFX.Value() * 1024 * 1024);
                        _platformSettings.heap[cap.secureGraphics[0].heapIndex].memcIndex = cap.secureGraphics[0].memcIndex;
                        _platformSettings.heap[cap.secureGraphics[0].heapIndex].heapType = NEXUS_HEAP_TYPE_SECURE_GRAPHICS;
                        _platformSettings.heap[cap.secureGraphics[0].heapIndex].memoryType = NEXUS_MEMORY_TYPE_ONDEMAND_MAPPED | NEXUS_MEMORY_TYPE_MANAGED | NEXUS_MEMORY_TYPE_SECURE;
                        /* TODO: allow securegfx to GFD1. for now, apply "-sd off" */
                        _serverSettings.session[0].output.sd = false;
                        _serverSettings.session[0].output.hd = true;
                        _serverSettings.session[0].output.encode = false;
                    }
                }
            }

            if (format != NEXUS_VideoFormat_eUnknown) {
                _serverSettings.display.format = format;
            }

            if (config.SVPType.IsSet() == true) {
                _serverSettings.svp = static_cast<nxserverlib_svp_type>(config.SVPType.Value());
            }

            /* display[1] will always be either SD or transcode */
            if (!_platformCapabilities.display[1].supported || _platformCapabilities.display[1].encoder) {
                for (unsigned int i = 0; i < NXCLIENT_MAX_SESSIONS; i++)
                    _serverSettings.session[i].output.sd = false;
            }
            /* display[0] will always be either HD, or system is headless */
            if (!_platformCapabilities.display[0].supported || _platformCapabilities.display[0].encoder) {
                for (unsigned int i = 0; i < NXCLIENT_MAX_SESSIONS; i++)
                    _serverSettings.session[i].output.hd = false;
            }

#if NEXUS_HAS_IR_INPUT
            if (config.IRMode.IsSet()) {
                TRACE_L1("setting ir_input_mode to %d (%s)", config.IRMode.Value(), config.IRMode.Data());

                for (unsigned int i = 0; i < NXCLIENT_MAX_SESSIONS; i++)
#if NEXUS_PLATFORM_VERSION_MAJOR < 16 || (NEXUS_PLATFORM_VERSION_MAJOR == 16 && NEXUS_PLATFORM_VERSION_MINOR < 3)
                    _serverSettings.session[i].ir_input_mode = config.IRMode.Value();
#else
                    for (unsigned int irInputIndex = 0; irInputIndex < NXSERVER_IR_INPUTS; irInputIndex++) {
                        _serverSettings.session[i].ir_input.mode[irInputIndex] = config.IRMode.Value();
                    }
#endif // NEXUS_PLATFORM_VERSION_MAJOR < 17
            } else {
                TRACE_L1("invalid ir_input_mode from config");
            }
#endif // NEXUS_HAS_IR_INPUT

            struct nxserver_cmdline_settings cmdline_settings;
            memset(&cmdline_settings, 0, sizeof(cmdline_settings));
            cmdline_settings.frontend = true;
            cmdline_settings.loudnessMode = static_cast<NEXUS_AudioLoudnessEquivalenceMode>(config.LoudnessMode.Value());
            _serverSettings.session[0].dolbyMs = nxserverlib_dolby_ms_type_none;

            if (config.FrameBufferWidth.IsSet() && config.FrameBufferHeight.IsSet()) {
                _serverSettings.fbsize.width = config.FrameBufferWidth.Value();
                _serverSettings.fbsize.height = config.FrameBufferHeight.Value();
            }

            if (config.HDCPLevel.Value() != Config::HDCP_NONE) {
                _serverSettings.hdcp.alwaysLevel = static_cast<NxClient_HdcpLevel>(config.HDCPLevel.Value());
                _serverSettings.hdcp.versionSelect = static_cast<NxClient_HdcpVersion>(config.HDCPVersion.Value());

                /* set the hdcp1x bin file path */
                if ((config.HDCP1xBinFile.IsSet() == true) && (config.HDCP1xBinFile.Value().empty() == false)) {
                    snprintf(_serverSettings.hdcp.hdcp1xBinFile, sizeof(_serverSettings.hdcp.hdcp1xBinFile), "%s", config.HDCP1xBinFile.Value().c_str());
                }

                /* set the hdcp2x bin file path */
                if ((config.HDCP2xBinFile.IsSet() == true) && (config.HDCP2xBinFile.Value().empty() == false)) {
                    snprintf(_serverSettings.hdcp.hdcp2xBinFile, sizeof(_serverSettings.hdcp.hdcp2xBinFile), "%s", config.HDCP2xBinFile.Value().c_str());
                }
            }

            rc = nxserver_modify_platform_settings(&_serverSettings,
                &cmdline_settings,
                &_platformSettings,
                &_serverSettings.memConfigSettings);
            if (rc == NEXUS_SUCCESS) {
                rc = NEXUS_Platform_MemConfigInit(&_platformSettings, &_serverSettings.memConfigSettings);

                if (rc == NEXUS_SUCCESS) {
                    BKNI_CreateMutex(&_lock);
                    _serverSettings.lock = _lock;

                    nxserver_set_client_heaps(&_serverSettings, &_platformSettings);

                    _serverSettings.client.connect = Platform::ClientConnect;
                    _serverSettings.client.disconnect = Platform::ClientDisconnect;

#if NEXUS_SERVER_HAS_EXTENDED_INIT
                    _instance = nxserverlib_init_extended(&_serverSettings, config.Authentication.Value());
#else
                    _instance = nxserverlib_init(&_serverSettings);
#endif

                    if (_instance != nullptr) {
                        rc = nxserver_ipc_init(_instance, _lock);

                        if (rc == NEXUS_SUCCESS) {
                            TRACE_L1("Created NexusServer[%p].\n", &_instance);
                        } else {
                            TRACE_L1("nxserver_ipc_init failed [%d]\n", rc);
                        }
                    } else {
                        TRACE_L1("nxserverlib_init_extended failed [%d]\n", rc);
                    }
                } else {
                    TRACE_L1("NEXUS_Platform_MemConfigInit failed [%d]\n", rc);
                }
            } else {
                TRACE_L1("nxserver_modify_platform_settings failed [%d]\n", rc);
            }

            StateChange(rc == NEXUS_SUCCESS ? OPERATIONAL : FAILURE);
        }

        ASSERT(_state != FAILURE);
    }

    // -------------------------------------------------------------------------------------------------------
    //   private methods
    // -------------------------------------------------------------------------------------------------------
    void Platform::Add(nxclient_t client, const NxClient_JoinSettings* joinSettings)
    {
        TRACE_L1("Nexus client %s connected... \n", joinSettings->name);

        if (_clientHandler != nullptr) {
            Exchange::IComposition::IClient* entry(Platform::Client::Create(client, joinSettings));
            _clientHandler->Attached(entry);
            entry->Release();
        }
    }

    void Platform::Remove(const char clientName[])
    {
        TRACE_L1("Nexus client %s disconnected... \n", clientName);

        if (_clientHandler != nullptr) {
            _clientHandler->Detached(clientName);
        }
    }

    void Platform::StateChange(server_state state)
    {
        TRACE_L1("Nexus server state change [%d -> %d] \n", _state, state);

        _state = state;

        if (_stateHandler != nullptr) {
            _stateHandler->StateChange(_state);
        }
    };

} // Broadcom

ENUM_CONVERSION_BEGIN(Broadcom::Config::svptype)

    { Broadcom::Config::NONE, _TXT("None") },
    { Broadcom::Config::VIDEO, _TXT("Video") },
    { Broadcom::Config::ALL, _TXT("All") },

    ENUM_CONVERSION_END(Broadcom::Config::svptype);

ENUM_CONVERSION_BEGIN(Broadcom::Config::hdcplevel)

    { Broadcom::Config::HDCP_NONE, _TXT("Hdcp_None") },
    { Broadcom::Config::HDCP_OPTIONAL, _TXT("Hdcp_Optional") },
    { Broadcom::Config::HDCP_MANDATORY, _TXT("Hdcp_Mandatory") },

    ENUM_CONVERSION_END(Broadcom::Config::hdcplevel);

ENUM_CONVERSION_BEGIN(Broadcom::Config::hdcpversion)

    { Broadcom::Config::AUTO, _TXT("Auto") },
    { Broadcom::Config::HDCP1X, _TXT("Hdcp1x") },
    { Broadcom::Config::HDCP22, _TXT("Hdcp22") },

    ENUM_CONVERSION_END(Broadcom::Config::hdcpversion);

ENUM_CONVERSION_BEGIN(Broadcom::Config::loudnessmode)

    { Broadcom::Config::LM_NONE,     _TXT("None")},
    { Broadcom::Config::LM_ATSC_A85, _TXT("ATSC_A85")},
    { Broadcom::Config::LM_EBU_R128, _TXT("EBU_R128")},

    ENUM_CONVERSION_END(Broadcom::Config::loudnessmode);

ENUM_CONVERSION_BEGIN(NEXUS_IrInputMode)

    { NEXUS_IrInputMode_eTwirpKbd, _TXT("NEXUS_IrInputMode_eTwirpKbd") },
    { NEXUS_IrInputMode_eSejin38KhzKbd, _TXT("NEXUS_IrInputMode_eSejin38KhzKbd") },
    { NEXUS_IrInputMode_eSejin56KhzKbd, _TXT("NEXUS_IrInputMode_eSejin56KhzKbd") },
    { NEXUS_IrInputMode_eRemoteA, _TXT("NEXUS_IrInputMode_eRemoteA") },
    { NEXUS_IrInputMode_eRemoteB, _TXT("NEXUS_IrInputMode_eRemoteB") },
    { NEXUS_IrInputMode_eCirGI, _TXT("NEXUS_IrInputMode_eCirGI") },
    { NEXUS_IrInputMode_eCirSaE2050, _TXT("NEXUS_IrInputMode_eCirSaE2050") },
    { NEXUS_IrInputMode_eCirTwirp, _TXT("NEXUS_IrInputMode_eCirTwirp") },
    { NEXUS_IrInputMode_eCirSony, _TXT("NEXUS_IrInputMode_eCirSony") },
    { NEXUS_IrInputMode_eCirRecs80, _TXT("NEXUS_IrInputMode_eCirRecs80") },
    { NEXUS_IrInputMode_eCirRc5, _TXT("NEXUS_IrInputMode_eCirRc5") },
    { NEXUS_IrInputMode_eCirUei, _TXT("NEXUS_IrInputMode_eCirUei") },
    { NEXUS_IrInputMode_eCirRfUei, _TXT("NEXUS_IrInputMode_eCirRfUei") },
    { NEXUS_IrInputMode_eCirEchoStar, _TXT("NEXUS_IrInputMode_eCirEchoStar") },
    { NEXUS_IrInputMode_eSonySejin, _TXT("NEXUS_IrInputMode_eSonySejin") },
    { NEXUS_IrInputMode_eCirNec, _TXT("NEXUS_IrInputMode_eCirNec") },
    { NEXUS_IrInputMode_eCirRC6, _TXT("NEXUS_IrInputMode_eCirRC6") },
    { NEXUS_IrInputMode_eCirGISat, _TXT("NEXUS_IrInputMode_eCirGISat") },
    { NEXUS_IrInputMode_eCustom, _TXT("NEXUS_IrInputMode_eCustom") },
    { NEXUS_IrInputMode_eCirDirectvUhfr, _TXT("NEXUS_IrInputMode_eCirDirectvUhfr") },
    { NEXUS_IrInputMode_eCirEchostarUhfr, _TXT("NEXUS_IrInputMode_eCirEchostarUhfr") },
    { NEXUS_IrInputMode_eCirRcmmRcu, _TXT("NEXUS_IrInputMode_eCirRcmmRcu") },
    { NEXUS_IrInputMode_eCirRstep, _TXT("NEXUS_IrInputMode_eCirRstep") },
    { NEXUS_IrInputMode_eCirXmp, _TXT("NEXUS_IrInputMode_eCirXmp") },
    { NEXUS_IrInputMode_eCirXmp2Ack, _TXT("NEXUS_IrInputMode_eCirXmp2Ack") },
    { NEXUS_IrInputMode_eCirRC6Mode0, _TXT("NEXUS_IrInputMode_eCirRC6Mode0") },
    { NEXUS_IrInputMode_eCirRca, _TXT("NEXUS_IrInputMode_eCirRca") },
    { NEXUS_IrInputMode_eCirToshibaTC9012, _TXT("NEXUS_IrInputMode_eCirToshibaTC9012") },
    { NEXUS_IrInputMode_eCirXip, _TXT("NEXUS_IrInputMode_eCirXip") },

    ENUM_CONVERSION_END(NEXUS_IrInputMode)

} // WPEFramework
