#include "PlatformImplementation.h"

#include <cstdint>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>

#include <thread>
#include <chrono>

#include <nexus_config.h>
#include <nexus_types.h>
#include <nexus_platform.h>
#include <nxclient.h>
#include <nxserverlib.h>
#include <nexus_display_vbi.h>

/* print_capabilities */
#if NEXUS_HAS_VIDEO_DECODER
#include <nexus_video_decoder.h>
#endif
#include <nexus_display.h>
#include <sys/stat.h>

#define Trace(fmt, args...) fprintf(stderr, "[pid=%d]: Platform [%s:%d] : " fmt, getpid(), __FILE__, __LINE__, ##args)

#ifdef __DEBUG__
#define Assert(x)                                                                                          \
    {                                                                                                      \
        if (!(x)) {                                                                                        \
            fprintf(stderr, "===== $$ [pid=%d]: Assert [%s:%d] (" #x ")\n", getpid(), __FILE__, __LINE__); \
            assert(x);                                                                                     \
        }                                                                                                  \
    }
#else
#define Assert(x)
#endif //__DEBUG__

BDBG_MODULE(NexusServer);

namespace WPEFramework {
namespace Broadcom {
    /* static */ Platform* Platform::_implementation = nullptr;

    /* static */ int Platform::ClientConnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings,
        NEXUS_ClientSettings* pClientSettings)
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

    /* static */ void Platform::HardwareReady(const uint8_t delay, Platform* parent)
    {
        Assert(parent != nullptr);

        Trace("Set PlatformReady in %ds \n", delay);

        std::this_thread::sleep_for(std::chrono::seconds(delay));

        if (parent != nullptr) {
            parent->PlatformReady();
        }
    }

    Platform::~Platform()
    {
        _state = DEINITIALIZING;

        nxserver_ipc_uninit();
        nxserverlib_uninit(_instance);
        BKNI_DestroyMutex(_lock);
        NEXUS_Platform_Uninit();

        pthread_cancel(_hardwareready.native_handle());
    }

    Platform::Platform(ICallback* callback, const bool authentication, const uint8_t hardwareDelay, const uint16_t graphicsHeap, const uint8_t boxMode, const uint16_t irMode)
        : _lock()
        , _instance()
        , _serverSettings()
        , _platformSettings()
        , _platformCapabilities()
        , _joinSettings()
        , _state(UNITIALIZED)
        , _nexusClients()
        , _clientHandler(callback)
    {
        NEXUS_Error rc;

        Assert(_implementation == nullptr);
        Assert(_instance == nullptr);
        Assert(_clientHandler != nullptr);

        _implementation = this;

        // Strange someone already started a NXServer.
        if (_instance == nullptr) {
            Trace("Start Nexus server...\n");

            if (boxMode != static_cast<uint8_t>(~0)) {
                // Set box mode using env var.
                char stringNumber[10];
                snprintf(stringNumber, sizeof(stringNumber), "%d", boxMode);
                ::setenv("B_REFSW_BOXMODE", stringNumber, 1);
                Trace("Set BoxMode to %d\n", boxMode);
            }

            NxClient_GetDefaultJoinSettings(&(_joinSettings));
            strcpy(_joinSettings.name, "NexusServerLocal");

            nxserver_get_default_settings(&(_serverSettings));
            NEXUS_Platform_GetDefaultSettings(&(_platformSettings));
            NEXUS_GetDefaultMemoryConfigurationSettings(&(_serverSettings.memConfigSettings));
            NEXUS_GetPlatformCapabilities(&(_platformCapabilities));


            if ((graphicsHeap > 0) && (graphicsHeap != static_cast<uint16_t>(~0))) {
                Trace("Set graphics heap to %dMB\n", graphicsHeap);
                _platformSettings.heap[NEXUS_MEMC0_GRAPHICS_HEAP].size = graphicsHeap * 1024 * 1024;
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
            for (unsigned int i = 0; i < NXCLIENT_MAX_SESSIONS; i++)
#if NEXUS_PLATFORM_VERSION_MAJOR < 16 || (NEXUS_PLATFORM_VERSION_MAJOR == 16 && NEXUS_PLATFORM_VERSION_MINOR < 3)
                _serverSettings.session[i].ir_input_mode = static_cast<NEXUS_IrInputMode>(irMode);
#else
                for (unsigned int irInputIndex = 0; i < NXSERVER_IR_INPUTS; i++)
                    _serverSettings.session[i].ir_input.mode[irInputIndex] = static_cast<NEXUS_IrInputMode>(irMode);
#endif // NEXUS_PLATFORM_VERSION_MAJOR < 17
#endif // NEXUS_HAS_IR_INPUT

            struct nxserver_cmdline_settings cmdline_settings;
            memset(&cmdline_settings, 0, sizeof(cmdline_settings));
            cmdline_settings.frontend = true;
            cmdline_settings.loudnessMode = NEXUS_AudioLoudnessEquivalenceMode_eNone;
            _serverSettings.session[0].dolbyMs = nxserverlib_dolby_ms_type_none;

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

                    _instance = nxserverlib_init_extended(&_serverSettings, authentication);
                    if (_instance != nullptr) {
                        rc = nxserver_ipc_init(_instance, _lock);

                        if (rc == NEXUS_SUCCESS) {
                            Trace("Created NexusServer[%p].\n", &_instance);
                        }
                        else {
                            Trace("nxserver_ipc_init failed [%d]\n", rc);
                        }
                    }
                    else {
                        Trace("nxserverlib_init_extended failed [%d]\n", rc);
                    }
                }
                else {
                    Trace("NEXUS_Platform_MemConfigInit failed [%d]\n", rc);
                }
            }
            else {
                Trace("nxserver_modify_platform_settings failed [%d]\n", rc);
            }
        }

        StateChange(rc == NEXUS_SUCCESS ? INITIALIZING : FAILURE);

        Assert(_state != FAILURE);

        if (_state != FAILURE) {
            _hardwareready = std::thread(HardwareReady, hardwareDelay, this);
        }
    }

    // -------------------------------------------------------------------------------------------------------
    //   private methods
    // -------------------------------------------------------------------------------------------------------
    void Platform::PlatformReady()
    {
        Trace("Set state to operational\n");
        StateChange(OPERATIONAL);
    }

    void Platform::Add(nxclient_t client, const NxClient_JoinSettings* joinSettings)
    {
        Trace("Nexus client %s connected... \n", joinSettings->name);

        _nexusClients.emplace(std::piecewise_construct,
            std::forward_as_tuple(joinSettings->name),
            std::forward_as_tuple(client, joinSettings));

        if (_clientHandler != nullptr) {
            _clientHandler->Attached(joinSettings->name);
        }
    }

    void Platform::Remove(const char clientName[])
    {
        Trace("Nexus client %s disconnected... \n", clientName);

        if (_clientHandler != nullptr) {
            _clientHandler->Detached(clientName);
        }
        _nexusClients.erase(clientName);
    }

    void Platform::StateChange(server_state state)
    {
        Trace("Nexus server state change [%d -> %d] \n", _state, state);

        _state = state;

        if (_clientHandler != nullptr) {
            _clientHandler->StateChange(_state);
        }
    };

    Platform::Client* Platform::GetClient(const ::std::string name)
    {
        Platform::Client* client(nullptr);

        std::map< ::std::string, Platform::Client>::iterator index(_nexusClients.begin());

        while ((index != _nexusClients.end()) && (name != index->second.Name())) {
            index++;
        }

        if (index != _nexusClients.end()) {
            client = &index->second;
        }

        return client;
    }
} // Broadcom
} // WPEFramework