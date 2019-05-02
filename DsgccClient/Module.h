#ifndef __MODULE_PLUGIN_DSGCCCLIENT_MODULE_H
#define __MODULE_PLUGIN_DSGCCCLIENT_MODULE_H

#ifndef MODULE_NAME
#define MODULE_NAME Plugin_DsgccClient
#endif

#include <core/core.h>
#include <plugins/plugins.h>
#include <tracing/tracing.h>
#include <interfaces/IDsgccClient.h>

#define TR()    TRACE_L1("%s::%s:%d", typeid(*this).name(), __FUNCTION__, __LINE__);

#undef EXTERNAL
#define EXTERNAL

#endif // __MODULE_PLUGIN_DSGCCCLIENT_MODULE_H
