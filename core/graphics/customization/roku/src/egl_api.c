#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/types.h>

#include "ion.h"
#include <drm_fourcc.h>
#include "gpu_aw.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <EGL/eglext.h>
#include <fbdev_window.h>
#include <pthread.h>
//#include <khronos/arm/winsys_fbdev/EGL/eglplatform.h>
//typedef void * EGLNativeDisplayType;

__thread gpu_surface_t *g_gpu2disp = NULL;
static EGLint window_attribute_list[] = {
    EGL_NONE
};

static const EGLint context_attribute_list[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

extern gpu_context_t gpu_con;

EGLSurface native_surface;
EGLContext native_context;

int32_t disp_fd = -1;
int32_t ion_fd = -1;

int count = 0;

/*
 * set layer config to display
 */
int set_layer(int current_fbo) {
	ATRACE_CALL()
	if (disp_fd <= 0) {
		disp_fd = open("/dev/disp", O_RDWR);
		if (disp_fd <= 0) {
			printf("aw: open /dev/disp failed\n");
			return -1;
		}
	}
	int current_mode;
	static int width = 0;
	static int height = 0;
	static int layer = 0;

	unsigned long ioctlParam[4] = { 0 };
	struct disp_device_config config;
	ioctlParam[0] = (unsigned long) 0;
	ioctlParam[1] = (unsigned long)&config;
	if(0==ioctl(disp_fd, DISP_DEVICE_GET_CONFIG, ioctlParam))
		current_mode = config.mode;
	else
		current_mode = DISP_TV_MOD_3840_2160P_30HZ;

	if((width == g_gpu2disp->width)&&(height == g_gpu2disp->height)&&(layer == g_gpu2disp->layer))
	{
		return 0;
	}
	else
	{
		width = g_gpu2disp->width;
		height = g_gpu2disp->height;
		layer = g_gpu2disp->layer;
	}


	memset(&g_gpu2disp->disp_config, 0, sizeof(g_gpu2disp->disp_config));
	//printf("aw: set_layer, sizeof(g_gpu2disp->disp_config):%d\n", sizeof(g_gpu2disp->disp_config));
	// initialize display config
	g_gpu2disp->disp_config.info.fb.format = DISP_FORMAT_RGBA_8888;
	g_gpu2disp->disp_config.info.fb.color_space = (g_gpu2disp->width < 720) ? DISP_BT601 : DISP_BT709;
	g_gpu2disp->disp_config.info.fb.size[0].width = g_gpu2disp->width;
	g_gpu2disp->disp_config.info.fb.size[0].height = g_gpu2disp->height;
	g_gpu2disp->disp_config.info.fb.size[1].width = g_gpu2disp->width;
	g_gpu2disp->disp_config.info.fb.size[1].height = g_gpu2disp->height;
	g_gpu2disp->disp_config.info.fb.size[2].width = g_gpu2disp->width;
	g_gpu2disp->disp_config.info.fb.size[2].height = g_gpu2disp->height;

	g_gpu2disp->disp_config.info.mode = LAYER_MODE_BUFFER;
	g_gpu2disp->disp_config.info.alpha_mode = 0;
	g_gpu2disp->disp_config.info.alpha_value = 0xff;
	g_gpu2disp->disp_config.info.fb.crop.x = 0;
	g_gpu2disp->disp_config.info.fb.crop.y = 0;
	g_gpu2disp->disp_config.info.fb.crop.width = (unsigned long long)g_gpu2disp->width << 32;
	g_gpu2disp->disp_config.info.fb.crop.height = (unsigned long long)g_gpu2disp->height << 32;
	g_gpu2disp->disp_config.info.fb.fd = g_gpu2disp->fd[current_fbo];
	//printf("aw: set_layer, width:%d, height:%d, fd:%d\n", g_gpu2disp->width, g_gpu2disp->height, g_gpu2disp->fd[current_fbo]);
	// set channel:2, layer:0, zorder:1, it will be on top of fb0
	if (g_gpu2disp->layer == 0) {
		g_gpu2disp->disp_config.info.screen_win.x = 0;
		g_gpu2disp->disp_config.info.screen_win.y = 0;
		g_gpu2disp->disp_config.info.screen_win.width = (current_mode==DISP_TV_MOD_3840_2160P_30HZ)?3840:1920;
		g_gpu2disp->disp_config.info.screen_win.height = (current_mode==DISP_TV_MOD_3840_2160P_30HZ)?2160:1080;
		g_gpu2disp->disp_config.channel = 2;
		g_gpu2disp->disp_config.enable = 1;
		g_gpu2disp->disp_config.layer_id = 0;
		g_gpu2disp->disp_config.info.zorder = 1;
	} else if (g_gpu2disp->layer == 1) {
		g_gpu2disp->disp_config.info.screen_win.x = 0;
		g_gpu2disp->disp_config.info.screen_win.y = 0;
		g_gpu2disp->disp_config.info.screen_win.width = (current_mode==DISP_TV_MOD_3840_2160P_30HZ)?3840:1920;
		g_gpu2disp->disp_config.info.screen_win.height = (current_mode==DISP_TV_MOD_3840_2160P_30HZ)?2160:1080;
		g_gpu2disp->disp_config.channel = 3;
		g_gpu2disp->disp_config.enable = 1;
		g_gpu2disp->disp_config.layer_id = 0;
		g_gpu2disp->disp_config.info.zorder = 2;
	}

	// set display config to display
	unsigned long args[4];
	args[0] = (unsigned long)0;
	args[1] = (unsigned long)(&g_gpu2disp->disp_config);
	//printf("aw: &g_gpu2disp->disp_config:%p\n", &g_gpu2disp->disp_config);
	args[2] = (unsigned long)1;
	args[3] = (unsigned long)0;
	int ret = ioctl(disp_fd, DISP_LAYER_SET_CONFIG2, (void *)args);
	if (ret != 0) {
		printf("aw: set display layer config2 failed, ret:%d\n", ret);
		return -1;
	}

	return 0;

}
struct fbdev_window native_window = {
    .width = 1280,
    .height = 720,
};


EGLDisplay eglGetDisplay(EGLNativeDisplayType display) {
	ATRACE_CALL()
    return gpu_con.egl.eglGetDisplay(display);
}

EGLDisplay eglGetPlatformDisplay(EGLenum platform, void * display,
                                 const EGLAttrib* attrib_list) {
	ATRACE_CALL()
	if (!gpu_con.egl.eglGetPlatformDisplay) {
		printf("eglGetPlatformDisplay  NULL\n");
		return NULL;
	}
	return gpu_con.egl.eglGetPlatformDisplay(platform, display, attrib_list);
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor) {
	ATRACE_CALL()
	if (!gpu_con.egl.eglInitialize) {
		printf("eglInitialize  NULL\n");
		return 0;
	}
	return gpu_con.egl.eglInitialize(dpy, major, minor);
}

EGLBoolean eglTerminate(EGLDisplay dpy) {
	ATRACE_CALL()
	if (!gpu_con.egl.eglTerminate) {
		printf("eglTerminate  NULL\n");
		return 0;
	}

	return gpu_con.egl.eglTerminate(dpy);
}

EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig* configs, EGLint config_size,
                         EGLint* num_config) {
	ATRACE_CALL()
    if (!gpu_con.egl.eglGetConfigs) {
	    printf("eglGetConfigs  NULL\n");
	    return 0;
    }

    return gpu_con.egl.eglGetConfigs(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs,
                           EGLint config_size, EGLint* num_config) {
	ATRACE_CALL()
    if (!gpu_con.egl.eglChooseConfig) {
	    printf("eglChooseConfig  NULL\n");
	    return 0;
    }

    return gpu_con.egl.eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value) {
	ATRACE_CALL()
	if (!gpu_con.egl.eglGetConfigAttrib) {
		printf("eglChooseConfig	NULL\n");
		return 0;
	}
	return gpu_con.egl.eglGetConfigAttrib(dpy, config, attribute, value);
}

int alloc_dma_buf(int size)
{
	int file_fd;
	int ret;
	void* addr_file,*addr_dma;

	if (ion_fd <= 0) {
		ion_fd = open("/dev/ion", O_RDWR);
		if(ion_fd <= 0)
		{
			printf("open /dev/ion err.\n");
			return -1;
		}
	}
	printf("aw: alloc_dma_buf size:%d\n", size);
	//alloc
    	struct ion_allocation_data sAllocInfo =
	{
		.len		= size,
		.align		= 4096,
		.heap_id_mask	= ION_HEAP_SYSTEM_MASK,
		.flags		= 0,
		.handle     = 0,
	};
	ret = ioctl(ion_fd, ION_IOC_ALLOC, &sAllocInfo);
	if(ret < 0)
	{
	    printf("alloc ion err.\n");
	    return  -1;
	}
    	//share
	struct ion_fd_data data = {
		.handle = sAllocInfo.handle,
	};

	ret = ioctl(ion_fd, ION_IOC_SHARE, &data);
	if (ret < 0)
		return ret;
	if (data.fd < 0) {
		printf("share ioctl returned negative fd\n");
		return -1;
	}
	struct ion_handle_data data2 = {
		.handle = sAllocInfo.handle,
	};
	ioctl(ion_fd, ION_IOC_FREE, &data2);

	return data.fd;
}

int init_ro_fbo(EGLDisplay dpy, gpu_surface_t* gpu2disp)
{

	int dma_fd;
	GLuint Tex, fbo;
	int i = 0;
	EGLImageKHR img;
	int size = gpu2disp->width * gpu2disp->height * 4;

	while(i < RO_SURFACE_NUM){
		dma_fd = alloc_dma_buf(size);
		if (dma_fd < 0)
			goto err;
		//just DRM_FORMAT_RGBA8888
		int atti = 0;
		EGLint attribs[30];
		attribs[atti++] = EGL_WIDTH;
		attribs[atti++] = gpu2disp->width;
		attribs[atti++] = EGL_HEIGHT;
		attribs[atti++] = gpu2disp->height;
		attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
		attribs[atti++] = DRM_FORMAT_RGBA8888;
		attribs[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
		attribs[atti++] = dma_fd;
		attribs[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
		attribs[atti++] = 0;
		attribs[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
		attribs[atti++] = gpu2disp->width * 4;

		attribs[atti++] = EGL_NONE;

		img = gpu_con.egl.eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, 0, attribs);
		if (img == EGL_NO_IMAGE_KHR) {
			printf("aw: eglCreateImageKHR failedi, eglerror:0x%x\n", eglGetError());
		}
		if (gpu_con.gl2.glGetError())
			printf("Error: failed: 0x%08X\n",gpu_con.gl2.glGetError());
		gpu_con.gl2.glGenTextures(1, &Tex);
		printf("aw: glGenTextures, Tex:%d\n", Tex);
		gpu_con.gl2.glBindTexture(GL_TEXTURE_EXTERNAL_OES, Tex);
		gpu_con.gl2.glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)img);
		if(gpu_con.gl2.glGetError())
			printf("Error: failed: 0x%08X\n", gpu_con.gl2.glGetError());

		gpu_con.gl2.glGenFramebuffers(1, &fbo);
		printf("aw: glGenFramebuffers, fbo:%d\n", fbo);
		if(gpu_con.gl2.glGetError())
			printf("Error: gen fbo failed: 0x%08X\n", gpu_con.gl2.glGetError());
		gpu_con.gl2.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		gpu_con.gl2.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES, Tex, 0);

		if(gpu_con.gl2.glGetError())
			printf("Error: glFramebufferTexture2D failed: 0x%08X\n", gpu_con.gl2.glGetError());

		gpu2disp->tex[i] = Tex;
		gpu2disp->fbo[i] = fbo;
		gpu2disp->fd[i] = dma_fd;
		gpu2disp->img[i] = img;
		printf("aw: The %d buffer has tex:%d, fbo:%d, fd:%d, img:%p, size %d\n", i, gpu2disp->tex[i], gpu2disp->fbo[i], gpu2disp->fd[i], gpu2disp->img[i], size);
		i++;

	}

	return 0;
err:
	if (i > 0) {
		gpu_con.gl2.glDeleteFramebuffers(i, gpu2disp->fbo);
		gpu_con.gl2.glDeleteTextures(i, gpu2disp->tex);
		while(i){
			close(gpu2disp->fd[i]);
			gpu_con.egl.eglDestroyImageKHR(dpy, gpu2disp->img[i]);
			i--;
		}
	}

	return -1;
}


EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType window,
                                  const EGLint* attrib_list) {
	ATRACE_CALL()
    int ret;
    EGLSurface surface = EGL_NO_SURFACE;

	if ((int)((int*)window) == 0) {
		//printf("aw: create native window surface(1280x720)\n");
		surface = gpu_con.egl.eglCreateWindowSurface(dpy, config, (EGLNativeWindowType)&native_window, attrib_list);
	} else if ((int)((int*)window) == 1) {
		return EGL_NO_SURFACE;
	} else {
		RoScreenDescription* desc = (RoScreenDescription*)window;
		printf("aw: eglCreateWindowSurface, width:%d, height:%d, layer:%d, buffers:%d, format:%d, preserved:%d\n",
			desc->width, desc->height, desc->layer, desc->buffers,
			desc->format, desc->preserved);
		// if global gpu_surface data haven't initialized, then create it here
		if (!g_gpu2disp) {
			g_gpu2disp = malloc(sizeof(struct gpu_surface));
			if (!g_gpu2disp) {
				printf("aw: failed to create gpu_surface data, sizeof:%d\n", sizeof(struct gpu_surface));
				goto err;
			}
		}

		//printf("aw: eglCreateWindowSurface 1\n");

		// if width, height and format have changed, then create new resources
		// and make a new configuration for display.
		if (g_gpu2disp->width != desc->width || g_gpu2disp->height != desc->height ||
			g_gpu2disp->format != desc->format) {
			//printf("aw: eglCreateWindowSurface 2\n");
			// release the old resources
			for (int i = 0; i < RO_SURFACE_NUM; i++) {
				// release texture resources
				if (g_gpu2disp->tex[i] > 0) {
					gpu_con.gl2.glDeleteTextures(1, &g_gpu2disp->tex[i]);
				}
				// release dma_buf
				if (g_gpu2disp->fd[i] > 0) {
					close(g_gpu2disp->fd[i]);
				}
				// release the eglimage resource
				if (g_gpu2disp->img[i] != EGL_NO_IMAGE_KHR) {
					gpu_con.egl.eglDestroyImageKHR(dpy, g_gpu2disp->img[i]);
				}
				// release fbo
				if (g_gpu2disp->fbo[i] > 0) {
					gpu_con.gl2.glDeleteFramebuffers(1, &g_gpu2disp->fbo[i]);
				}
			}
			// initialize resource data
			g_gpu2disp->width = desc->width;
			g_gpu2disp->height = desc->height;
			g_gpu2disp->format = desc->format;
			g_gpu2disp->layer = desc->layer;
			g_gpu2disp->need_recreate = 1;


		}
		g_gpu2disp->use_fbo = 1;
		g_gpu2disp->current_fbo = 0;
		surface = gpu_con.egl.eglGetCurrentSurface(EGL_DRAW);
		if (surface == EGL_NO_SURFACE) {
			//printf("aw: eglCreateWindowSurface 3\n");
			surface = gpu_con.egl.eglCreatePbufferSurface(dpy, config, attrib_list);
			if (surface == EGL_NO_SURFACE) {
				printf("aw: cannot create pbuffer surface\n");
				goto err;
			}
		}
	}

	return surface;
err:
	if (g_gpu2disp) {
		free(g_gpu2disp);
	}
	printf("aw: failed to creat window: %d\n", (int)((int *)window));

	return EGL_NO_SURFACE;
}

EGLSurface eglCreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config, void* native_window,
                                          const EGLAttrib* attrib_list) {
	ATRACE_CALL()
	if (!gpu_con.egl.eglCreatePlatformWindowSurface) {
		printf("eglCreatePlatformWindowSurface NULL\n");
		return EGL_NO_SURFACE;
	}

	return gpu_con.egl.eglCreatePlatformWindowSurface(dpy, config, native_window, attrib_list);
}

EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap,
                                  const EGLint* attrib_list) {
	ATRACE_CALL()
	if (!gpu_con.egl.eglCreatePixmapSurface) {
	    printf("eglCreatePixmapSurface NULL\n");
	    return EGL_NO_SURFACE;
	}

    return gpu_con.egl.eglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLSurface eglCreatePlatformPixmapSurface(EGLDisplay dpy, EGLConfig config, void* native_pixmap,
                                          const EGLAttrib* attrib_list) {
	ATRACE_CALL()
	if (!gpu_con.egl.eglCreatePlatformPixmapSurface) {
		printf("eglCreatePlatformPixmapSurface NULL\n");
		return EGL_NO_SURFACE;
	}

	return gpu_con.egl.eglCreatePlatformPixmapSurface(dpy, config, native_pixmap, attrib_list);
}

EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint* attrib_list) {
	ATRACE_CALL()

    return gpu_con.egl.eglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface) {
	ATRACE_CALL()

	// release all the resources
	if (g_gpu2disp) {
		gpu_con.gl2.glDeleteFramebuffers(RO_SURFACE_NUM, g_gpu2disp->fbo);
		gpu_con.gl2.glDeleteTextures(RO_SURFACE_NUM, g_gpu2disp->tex);

		// disable display layer
		if (g_gpu2disp->use_fbo) {
			if (g_gpu2disp->layer == 0) {
				g_gpu2disp->disp_config.channel = 2;
				g_gpu2disp->disp_config.enable = 0;
				g_gpu2disp->disp_config.layer_id = 0;
				g_gpu2disp->disp_config.info.zorder = 1;
			} else if (g_gpu2disp->layer == 1) {
				g_gpu2disp->disp_config.channel = 3;
				g_gpu2disp->disp_config.enable = 0;
				g_gpu2disp->disp_config.layer_id = 0;
				g_gpu2disp->disp_config.info.zorder = 2;
			}
			// set display config to display
			unsigned long args[4];
			args[0] = (unsigned long)0;
			args[1] = (unsigned long)(&g_gpu2disp->disp_config);
			//printf("aw: eglDestroySurface\n");
			args[2] = (unsigned long)1;
			args[3] = (unsigned long)0;
			int ret = ioctl(disp_fd, DISP_LAYER_SET_CONFIG2, (void *)args);
			printf("aw: set display layer config2 failed, ret:%d\n", ret);
			if (ret != 0) {
				printf("aw: set display layer config2 failed, ret:%d\n", ret);
				return -1;
			}
		}

		int i = 0;
		while(i < RO_SURFACE_NUM) {
			close(g_gpu2disp->fd[i]);
			gpu_con.egl.eglDestroyImageKHR(dpy, g_gpu2disp->img[i]);
			i++;
		}
		close(ion_fd);
		close(disp_fd);
		ion_fd = -1;
		disp_fd = -1;
		free(g_gpu2disp);
		g_gpu2disp = NULL;
	}

	if (surface != EGL_NO_SURFACE) {
		return gpu_con.egl.eglDestroySurface(dpy, surface);
	}
}

EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value) {
	ATRACE_CALL()

    return gpu_con.egl.eglQuerySurface(dpy, surface, attribute, value);
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_list,
                            const EGLint* attrib_list) {
	ATRACE_CALL()

		EGLContext context = gpu_con.egl.eglCreateContext(dpy, config, share_list, attrib_list);

    return context;
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
	ATRACE_CALL()

    return gpu_con.egl.eglDestroyContext(dpy, ctx);
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
	ATRACE_CALL()
	EGLBoolean ret;
	if (g_gpu2disp == NULL || (draw != EGL_NO_SURFACE && g_gpu2disp != NULL && g_gpu2disp->use_fbo == 1 && gpu_con.egl.eglGetCurrentSurface(EGL_DRAW) == EGL_NO_SURFACE)) {
		//printf("aw++++++++++++++++++++++: eglMakeCurrent, dpy:%p, draw:%p, ctx:%p, eglGetError():0x%x\n", dpy, draw, ctx, eglGetError());
		ret = gpu_con.egl.eglMakeCurrent(dpy, draw, read, ctx);
	}

	if (g_gpu2disp != NULL && g_gpu2disp->need_recreate == 1) {
		// create new resources
		if (init_ro_fbo(dpy, g_gpu2disp)) {
			printf("aw: init_ro_fbo failed\n");
			return EGL_FALSE;
		}
		//printf("aw: eglMakeCurrent, current_fbo:%d, fd:%d\n", g_gpu2disp->current_fbo, g_gpu2disp->fbo[g_gpu2disp->current_fbo]);
		gpu_con.gl2.glBindFramebuffer(GL_FRAMEBUFFER, g_gpu2disp->fbo[g_gpu2disp->current_fbo]);
	}

	return ret;
}

EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint* value) {
	ATRACE_CALL()

    return gpu_con.egl.eglQueryContext(dpy, ctx, attribute, value);
}

EGLContext eglGetCurrentContext(void) {
	ATRACE_CALL()

    return gpu_con.egl.eglGetCurrentContext();
}

EGLSurface eglGetCurrentSurface(EGLint readdraw) {
	ATRACE_CALL()
    return gpu_con.egl.eglGetCurrentSurface(readdraw);
}

EGLDisplay eglGetCurrentDisplay(void) {
	ATRACE_CALL()

    return gpu_con.egl.eglGetCurrentDisplay();
}

EGLBoolean eglWaitGL(void) {
	ATRACE_CALL()

    return gpu_con.egl.eglWaitGL();
}

EGLBoolean eglWaitNative(EGLint engine) {
	ATRACE_CALL()
    return gpu_con.egl.eglWaitNative(engine);
}

EGLint eglGetError(void) {

    return gpu_con.egl.eglGetError();
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char* procname) {
	ATRACE_CALL()
    // eglGetProcAddress() could be the very first function called
    // in which case we must make sure we've initialized ourselves, this
    // happens the first time egl_get_display() is called.


    return gpu_con.egl.eglGetProcAddress(procname);
}

EGLBoolean eglSwapBuffersWithDamageKHR(EGLDisplay dpy, EGLSurface draw, EGLint* rects,
                                       EGLint n_rects) {
    ATRACE_CALL();

	if (g_gpu2disp != NULL && g_gpu2disp->use_fbo == 1) {
		gpu_con.gl2.glFinish();
		int ret = set_layer(g_gpu2disp->current_fbo);
		if (ret != 0) {
			printf("aw: set_layer failed, ret:%d\n", ret);
			return EGL_FALSE;
		}
		g_gpu2disp->current_fbo++;
		g_gpu2disp->current_fbo %= RO_SURFACE_NUM;
		gpu_con.gl2.glBindFramebuffer(GL_FRAMEBUFFER, g_gpu2disp->fbo[g_gpu2disp->current_fbo]);
	}
    return gpu_con.egl.eglSwapBuffersWithDamageKHR(dpy, draw, rects, n_rects);
}

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
	ATRACE_CALL();

	if (g_gpu2disp != NULL && g_gpu2disp->use_fbo == 1) {
		gpu_con.gl2.glFinish();

// debug: for get every frame rendered by gpu
#if 0
	{
		static __thread int count = 0;
		char str[128];
		sprintf(str, "pixel_%d_frame_%d.dat", syscall(__NR_gettid), count);

		unsigned char* data = malloc(1280 * 720 * 4);
		glReadPixels(0, 0, 1280, 720, GL_RGBA, GL_UNSIGNED_BYTE, data);
		FILE* fp = fopen(str, "wb");
		fwrite(data, 4, 1280 * 720, fp);
		fclose(fp);
		free(data);
		count++;
	}
#endif
		int ret = set_layer(g_gpu2disp->current_fbo);
		if (ret != 0) {
			printf("aw: set_layer failed, ret:%d\n", ret);
			return EGL_FALSE;
		}
		g_gpu2disp->current_fbo++;
		g_gpu2disp->current_fbo %= RO_SURFACE_NUM;
		//printf("aw: eglSwapBuffers, current_fbo:%d, fd:%d\n", g_gpu2disp->current_fbo, g_gpu2disp->fbo[g_gpu2disp->current_fbo]);
		gpu_con.gl2.glBindFramebuffer(GL_FRAMEBUFFER, g_gpu2disp->fbo[g_gpu2disp->current_fbo]);
	}

	return gpu_con.egl.eglSwapBuffers(dpy, surface);
}

const char* eglQueryString(EGLDisplay dpy, EGLint name) {
	ATRACE_CALL()

    return gpu_con.egl.eglQueryString(dpy, name);
}

EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value) {
	ATRACE_CALL()

	return gpu_con.egl.eglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
	ATRACE_CALL()

	return gpu_con.egl.eglBindTexImage(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
	ATRACE_CALL()

	return gpu_con.egl.eglReleaseTexImage(dpy, surface, buffer);
}

EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval) {
	ATRACE_CALL()

    return gpu_con.egl.eglSwapInterval(dpy, interval);
}

EGLBoolean eglWaitClient(void) {
	ATRACE_CALL()

    return gpu_con.egl.eglWaitClient();
}

EGLBoolean eglBindAPI(EGLenum api) {
	ATRACE_CALL()

    return gpu_con.egl.eglBindAPI(api);
}

EGLenum eglQueryAPI(void) {
	ATRACE_CALL()

    return gpu_con.egl.eglQueryAPI();
}

EGLBoolean eglReleaseThread(void) {
	ATRACE_CALL()

    return gpu_con.egl.eglReleaseThread();
}

EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
                                            EGLConfig config, const EGLint* attrib_list) {
	ATRACE_CALL()

    return gpu_con.egl.eglCreatePbufferFromClientBuffer(dpy, buftype, buffer, config,
                                                          attrib_list);
}

EGLBoolean eglLockSurfaceKHR(EGLDisplay dpy, EGLSurface surface, const EGLint* attrib_list) {
	ATRACE_CALL()

	return gpu_con.egl.eglLockSurfaceKHR(dpy, surface, attrib_list);
}

EGLBoolean eglUnlockSurfaceKHR(EGLDisplay dpy, EGLSurface surface) {
	ATRACE_CALL()

	return gpu_con.egl.eglUnlockSurfaceKHR(dpy, surface);
}

EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                              EGLClientBuffer buffer, const EGLint* attrib_list) {
	ATRACE_CALL()

    return gpu_con.egl.eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);
}

EGLImage eglCreateImage(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer,
                        const EGLAttrib* attrib_list) {
	ATRACE_CALL()

    return gpu_con.egl.eglCreateImage(dpy, ctx, target, buffer, attrib_list);
}

EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR img) {
	ATRACE_CALL()

    return gpu_con.egl.eglDestroyImageKHR(dpy, img);
}

EGLBoolean eglDestroyImage(EGLDisplay dpy, EGLImageKHR img) {
	ATRACE_CALL()

    return gpu_con.egl.eglDestroyImage(dpy, img);
}

// ----------------------------------------------------------------------------
// EGL_EGLEXT_VERSION 5
// ----------------------------------------------------------------------------

EGLSyncKHR eglCreateSync(EGLDisplay dpy, EGLenum type, const EGLAttrib* attrib_list) {
	ATRACE_CALL()

    return gpu_con.egl.eglCreateSync(dpy, type, attrib_list);
}

EGLSyncKHR eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint* attrib_list) {
	ATRACE_CALL()

    return gpu_con.egl.eglCreateSyncKHR(dpy, type, attrib_list);
}

EGLBoolean eglDestroySync(EGLDisplay dpy, EGLSyncKHR sync) {
	ATRACE_CALL()

    return gpu_con.egl.eglDestroySync(dpy, sync);
}

EGLBoolean eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) {
	ATRACE_CALL()

    return gpu_con.egl.eglDestroySyncKHR(dpy, sync);
}

EGLBoolean eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode) {
	ATRACE_CALL()

    return gpu_con.egl.eglSignalSyncKHR(dpy, sync, mode);
}

EGLint eglClientWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTimeKHR timeout) {
	ATRACE_CALL()

    return gpu_con.egl.eglClientWaitSyncKHR(dpy, sync, flags, timeout);
}

EGLint eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout) {
	ATRACE_CALL()

    return gpu_con.egl.eglClientWaitSyncKHR(dpy, sync, flags, timeout);
}

EGLBoolean eglGetSyncAttrib(EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib* value) {
	ATRACE_CALL()

    return gpu_con.egl.eglGetSyncAttrib(dpy, sync, attribute, value);
}

EGLBoolean eglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint* value) {
	ATRACE_CALL()

    return gpu_con.egl.eglGetSyncAttribKHR(dpy, sync, attribute, value);
}

EGLStreamKHR eglCreateStreamKHR(EGLDisplay dpy, const EGLint* attrib_list) {
	ATRACE_CALL()

    return gpu_con.egl.eglCreateStreamKHR(dpy, attrib_list);
}

EGLBoolean eglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	ATRACE_CALL()

    return gpu_con.egl.eglDestroyStreamKHR(dpy, stream);
}

EGLBoolean eglStreamAttribKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute,
                              EGLint value) {
	ATRACE_CALL()

    return gpu_con.egl.eglStreamAttribKHR(dpy, stream, attribute, value);
}

EGLBoolean eglQueryStreamKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute,
                             EGLint* value) {
	ATRACE_CALL()

    return gpu_con.egl.eglQueryStreamKHR(dpy, stream, attribute, value);
}

EGLBoolean eglQueryStreamu64KHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute,
                                EGLuint64KHR* value) {
	ATRACE_CALL()

    return gpu_con.egl.eglQueryStreamu64KHR(dpy, stream, attribute, value);
}

EGLBoolean eglQueryStreamTimeKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute,
                                 EGLTimeKHR* value) {
	ATRACE_CALL()

    return gpu_con.egl.eglQueryStreamTimeKHR(dpy, stream, attribute, value);
}

EGLSurface eglCreateStreamProducerSurfaceKHR(EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream,
                                             const EGLint* attrib_list) {
	ATRACE_CALL()

    return gpu_con.egl.eglCreateStreamProducerSurfaceKHR(dpy, config, stream, attrib_list);
}

EGLBoolean eglStreamConsumerGLTextureExternalKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	ATRACE_CALL()

    return gpu_con.egl.eglStreamConsumerGLTextureExternalKHR(dpy, stream);
}

EGLBoolean eglStreamConsumerAcquireKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	ATRACE_CALL()

    return gpu_con.egl.eglStreamConsumerAcquireKHR(dpy, stream);
}

EGLBoolean eglStreamConsumerReleaseKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	ATRACE_CALL()

    return gpu_con.egl.eglStreamConsumerReleaseKHR(dpy, stream);
}

EGLNativeFileDescriptorKHR eglGetStreamFileDescriptorKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	ATRACE_CALL()

    return gpu_con.egl.eglGetStreamFileDescriptorKHR(dpy, stream);
}

EGLStreamKHR eglCreateStreamFromFileDescriptorKHR(EGLDisplay dpy,
                                                  EGLNativeFileDescriptorKHR file_descriptor) {
	ATRACE_CALL()

    return gpu_con.egl.eglCreateStreamFromFileDescriptorKHR(dpy, file_descriptor);
}

EGLint eglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags) {
	ATRACE_CALL()

    return gpu_con.egl.eglWaitSyncKHR(dpy, sync, flags);
}

EGLBoolean eglWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags) {
	ATRACE_CALL()

    return gpu_con.egl.eglWaitSync(dpy, sync, flags);
}

EGLBoolean eglSetDamageRegionKHR(EGLDisplay dpy, EGLSurface surface, EGLint* rects,
                                 EGLint n_rects) {
	ATRACE_CALL()

    return gpu_con.egl.eglSetDamageRegionKHR(dpy, surface, rects, n_rects);
}

