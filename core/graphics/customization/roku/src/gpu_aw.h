#ifndef _GPU_AW_H
#define _GPU_AW_H

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <GLES2/gl2.h>
#include <GLES3/gl32.h>

#include <EGL/eglext.h>

#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>
#include "sunxi_display2.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>


typedef struct gpu_context{
	#undef GL_ENTRY
	#undef EGL_ENTRY
	struct {
		#define EGL_ENTRY(_r, _api, ...) _r (*(_api))(__VA_ARGS__);
		#include "egl_entries.h"
	} egl;
	struct {
		#define GL_ENTRY(_r, _api, ...) _r (*(_api))(__VA_ARGS__);
		#include "entries.h"
	}gl2;
	#undef GL_ENTRY
	#undef EGL_ENTRY
	int init;
	int debug;
} gpu_context_t;

#define RO_SURFACE_NUM 2
typedef enum {
    RGBA8888 = 0,  // 32 bit RGBA, 8 bits each component
    RGB888,        // 24 bit RGB, 8 bits each component, packed into 32 bit
    RGB565,        // 16 bit RGB
    RGBA5551,      // 16 bit RGBA, 5 bit color components, 1 bit alpha
    RGBA4444,      // 16 bit RGBA, 4 bit components
    ALPHA8,        // 8 bit alpha surface, fixed color
    ETC1RGB8,      // ETC1 compressed image
    UYVY,          // 16 bit YUV 4:2:2
    YUY2,          // 16 bit YUV 4:2:2 YCbYCr
    UNKNOWN_gg        // should be last
} RoPixelFormat;

#define UNSUPPORT_FORMAT  0

static int drm_format_map[UNKNOWN_gg + 1][3] = {
	{DRM_FORMAT_RGBA8888, 32, 1},
	{DRM_FORMAT_RGB888, 24, 1},
	{DRM_FORMAT_RGB565,16,1},
	{DRM_FORMAT_RGBA5551,16,1},
	{DRM_FORMAT_RGBA4444,16,1},
	{UNSUPPORT_FORMAT,0,0},
	{UNSUPPORT_FORMAT,0,0},
	{DRM_FORMAT_UYVY,16,2},
	{DRM_FORMAT_YUYV,16,2},
	{UNSUPPORT_FORMAT,0,0}
};

typedef struct {
    int32_t width;  /* native screen width */
    int32_t height; /* native screen height */
    int8_t layer;   /* video, graphics, or overlay */
    int8_t buffers; /* single, double, or triple buffering */
    RoPixelFormat format;
    int preserved; /* enable EGL buffer preservation */
} RoScreenDescription;


typedef struct gpu_surface {
	int32_t width;
	int32_t height;
	RoPixelFormat format;
	int8_t layer;
	int ion_fd;
	RoScreenDescription RoScreen;
	GLuint tex[RO_SURFACE_NUM];
	int fd[RO_SURFACE_NUM];
	GLuint fbo[RO_SURFACE_NUM];
	uint32_t current_fbo;
	int use_fbo;
	EGLImageKHR img[RO_SURFACE_NUM];
	int need_recreate;
	int disp_fd;
	struct disp_layer_config2 disp_config;
} gpu_surface_t;

#if 0
#define ATRACE_CALL() \
	{ \
		printf("aw: %s, pid:%d, tid:%d\n",__func__, getpid(), syscall(__NR_gettid)); \
	}
#else
#define ATRACE_CALL()
#endif
#endif

