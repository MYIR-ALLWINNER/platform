#include <dlfcn.h>
#include <stdio.h>

#include "gpu_aw.h"

#define MALI_LIBRARY  "libmali.so"

gpu_context_t gpu_con;

#undef GL_ENTRY
#undef EGL_ENTRY
#define GL_ENTRY(_r, _api, ...) #_api,
#define EGL_ENTRY(_r, _api, ...) #_api,

char *gl2_names[] = {
    #include "entries.h"
    NULL
};

char *egl_names[] = {
    #include "egl_entries.h"
    NULL
};
typedef __eglMustCastToProperFunctionPointerType (* getProcAddressType)(const char*);

getProcAddressType getProcAddress = NULL;

#undef GL_ENTRY
#undef EGL_ENTRY


static void __attribute__((constructor))hal_Init(void) {
    char task_name[50];
	void *handle_gpu;

    pid_t pid = getpid();
	handle_gpu = dlopen(MALI_LIBRARY, RTLD_LAZY);
	if (handle_gpu== NULL) {
		printf("dlopen gpu so err.\n");
		return;
	}
	getProcAddress = (getProcAddressType)dlsym(handle_gpu, "eglGetProcAddress");
	if (getProcAddress == NULL)
		printf("eglGetProcAddress got NULL\n");
	int i = 1;
	char **api = egl_names;
	__eglMustCastToProperFunctionPointerType* curr =
            (__eglMustCastToProperFunctionPointerType*)&gpu_con.egl;
retry:
        while (*api) {
            char const * name = *api;
            __eglMustCastToProperFunctionPointerType f =
                (__eglMustCastToProperFunctionPointerType)dlsym(handle_gpu, name);
            if (f == NULL && getProcAddress) {
                // couldn't find the entry-point, use eglGetProcAddress()
                f = getProcAddress(name);
                if (f == NULL) {
                    f = (__eglMustCastToProperFunctionPointerType)NULL;
                }
            }
	    if (NULL == f) {
		    //printf("%s got NULL\n",api);
	    }
            *curr++ = f;
            api++;
        }

	if (i) {
		api = gl2_names;
		i = 0;
		curr = (__eglMustCastToProperFunctionPointerType*)&gpu_con.gl2;
		goto retry;
	}

}



