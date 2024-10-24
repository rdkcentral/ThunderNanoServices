#pragma once

#include "Module.h"

extern "C" {

struct platform;

extern platform* script_create_platform(const char configuration[]);
extern void      script_destroy_platform(platform*);

extern uint32_t  script_prepare(platform*, const uint32_t length, const char script[]);
extern uint32_t  script_execute(platform*);
extern uint32_t  script_abort(platform*);

}
