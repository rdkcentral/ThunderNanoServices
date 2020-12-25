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

#pragma once

#include <gst/gst.h>

namespace WPEFramework {
namespace Player {
    namespace Implementation {
        namespace {
            namespace GstCallbacks {
                gboolean gstBusCallback(GstBus* bus, GstMessage* message, CENC::PipelineData* data)
                {
                    switch (GST_MESSAGE_TYPE(message)) {
                    case GST_MESSAGE_ERROR: {

                        GError* err;
                        gchar* debugInfo;
                        gst_message_parse_error(message, &err, &debugInfo);
                        TRACE_GLOBAL(Trace::Error, (_T("Error received from element %s: %s\n"), gst_object_get_name(message->src), err->message));
                        TRACE_GLOBAL(Trace::Error, (_T("Debugging information: %s\n"), debugInfo ? debugInfo : "none"));
                        g_clear_error(&err);
                        g_free(debugInfo);

                        gst_element_set_state(data->_playbin, GST_STATE_NULL);
                        g_main_loop_quit(data->_mainLoop);
                        break;
                    }
                    case GST_MESSAGE_EOS: {
                        TRACE_GLOBAL(Trace::Information, (_T("Reached end of stream")));
                        gst_element_set_state(data->_playbin, GST_STATE_NULL);
                        break;
                    }
                    case GST_MESSAGE_STATE_CHANGED: {
                        GstState old_state, new_state;
                        gst_message_parse_state_changed(message, &old_state, &new_state, NULL);
                        std::string old_str(gst_element_state_get_name(old_state)), new_str(gst_element_state_get_name(new_state));
                        std::string filename("cencplayer-" + old_str + "-" + new_str);
                        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(data->_playbin), GST_DEBUG_GRAPH_SHOW_ALL, filename.c_str());
                        break;
                    }
                    default:
                        break;
                    }
                    return TRUE;
                }
            }
        }
    }
}
}
