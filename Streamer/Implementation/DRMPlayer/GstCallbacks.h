#pragma once

#include "DRMPlayer.h"
#include "GstOcdmDecrypt.h"
#include <gst/gst.h>

namespace WPEFramework {
namespace Player {
    namespace Implementation {
        namespace {
            namespace GstCallbacks {
                gboolean gstBusCallback(GstBus* bus, GstMessage* message, DRMPlayer::PipelineData* data)
                {
                    switch (GST_MESSAGE_TYPE(message)) {
                    case GST_MESSAGE_ERROR: {

                        GError* err;
                        gchar* debugInfo;
                        gst_message_parse_error(message, &err, &debugInfo);
                        TRACE_L1("Error received from element %s: %s\n", gst_object_get_name(message->src), err->message);
                        TRACE_L1("Debugging information: %s\n", debugInfo ? debugInfo : "none");
                        g_clear_error(&err);
                        g_free(debugInfo);

                        gst_element_set_state(data->_playbin, GST_STATE_NULL);
                        g_main_loop_quit(data->_mainLoop);
                        break;
                    }
                    case GST_MESSAGE_EOS: {
                        TRACE_L1("Reached end of stream");
                        gst_element_set_state(data->_playbin, GST_STATE_NULL);
                        break;
                    }
                    default:
                        break;
                    }
                    return TRUE;
                }

                static gboolean plugin_init(GstPlugin* plugin)
                {
                    return gst_element_register(plugin, "ocdmdecrypt", GST_RANK_PRIMARY,
                        GST_TYPE_OCDMDECRYPT);
                }
            } // namespace GstCallbacks
        }
    } // namespace Implementation
} // namespace Player
} // namespace WPEFramework