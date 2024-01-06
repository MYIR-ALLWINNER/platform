/*
 ** Copyright 2007, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <EGL/eglext.h>

#include "gpu_aw.h"



// ----------------------------------------------------------------------------
// Actual GL entry-points
// ----------------------------------------------------------------------------
extern gpu_context_t gpu_con;

#undef API_ENTRY
#undef CALL_GL_API
#undef CALL_GL_API_RETURN

#define API_ENTRY(_api) _api

#define CALL_GL_API_RETURN(_api, ...)         \
        if (gpu_con.gl2._api != NULL) return gpu_con.gl2._api(__VA_ARGS__);

#define CALL_GL_API(_api, ...) \
	CALL_GL_API_RETURN(_api, __VA_ARGS__)


#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "gl2_api_entry.h"
#include "gl2ext_api_entry.h"
#pragma GCC diagnostic warning "-Wunused-parameter"

#undef API_ENTRY
#undef CALL_GL_API
#undef CALL_GL_API_INTERNAL_CALL
#undef CALL_GL_API_RETURN
