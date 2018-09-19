#ifndef __MODULE_COMPOSITION_IMPLEMENTATION_H
#define __MODULE_COMPOSITION_IMPLEMENTATION_H

#ifndef MODULE_NAME
#define MODULE_NAME Compositor_Implementation
#endif

#include <core/core.h>
#include <tracing/tracing.h>

#ifdef __WIN32__
#undef EXTERNAL
#ifdef __MODULE_COM__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_COMPOSITION_IMPLEMENTATION_H
