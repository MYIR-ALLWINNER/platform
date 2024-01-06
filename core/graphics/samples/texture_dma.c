/*
 * This confidential and proprietary software should be used
 * under the licensing agreement from Allwinner Technology.
 *
 * Copyright (C) 2020-2030 Allwinner Technology Limited
 * All rights reserved.
 *
 * Author: Albert zhengwanyu <zhengwanyu@allwinnertech.com>
 *
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from Allwinner Technology Limited.
 */

/*
 * This program is mainly for window/pubuffer surface, FBO, texture operations
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <linux/fb.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>

#include "ion.h"
#include "drm_fourcc.h"

//#define ON_SCREEN
#define __ARM__GPU__

#ifdef __ARM__GPU__
#include <EGL/mali_fbdev_types.h>
#else
#include <EGL/fbdev_window.h>
#endif

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

struct FormatName {
	unsigned int drm_format;
	char name[10];
};

struct Shader {
	GLuint id;
};

struct ion_mem
{
	unsigned int phy;     /* physical address */
	unsigned char *virt;  /* virtual address */
	unsigned long size;   /* ion buffer size */
	int dmafd;            /* ion dmabuf fd */
	int handle;           /* ion handle */
};

struct Renderer{
	//EGL References
	EGLint egl_major, egl_minor;
	EGLDisplay egl_display;
	EGLContext egl_context;
	EGLSurface egl_surface;

	//Shader references
	struct Shader vs;
	struct Shader fs;
	GLuint shader_program;

	//ion dma references
	struct ion_mem TexMem;
	struct ion_mem FboMem;

	/*Texure references*/
	GLuint Tex;
	GLuint FboColorBufferTex;

	GLuint fbo;
};

struct TestCase {
	struct Renderer renderer;

	char TexPath[50];			//path of texture file
	unsigned int TexFormat; 		//pixel format of texture

	unsigned int tex_width, tex_height;     //size of texture
	unsigned int view_width, view_height;   //size of viewport
	int ion_fd;

	unsigned int OutFormat;			//output pixel format
	char OutPath[50];			//path of output file
};


struct FormatName DrmFormat[] = {
	{DRM_FORMAT_ARGB8888, "argb8888"}, //fbdev、LCD default format
	{DRM_FORMAT_RGBA8888, "rgba8888"},
	{DRM_FORMAT_BGRA8888, "bgrb8888"},
	{DRM_FORMAT_NV21,     "nv21"    },
	{DRM_FORMAT_NV12,     "nv12"    },
};



struct fbdev_window native_window;

/*PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
*/

static const char *vertex_shader_source =
	"attribute vec4 aPosition;                               \n"
	"attribute vec2 aTexCoord;                               \n"
//	"attribute vec4 aColor;                                  \n"

	"varying vec2 vTexCoord;                                 \n"
//	"varying vec4 vColor;                                    \n"

	"void main()                                             \n"
	"{                                                       \n"
	"	vTexCoord = aTexCoord;                           \n"
//	"	vColor = aColor;                                 \n"

	"	gl_Position = aPosition;                         \n"
	"}                                                       \n";

static const char *fragment_shader_source =
	"#extension GL_OES_EGL_image_external : require          \n"
	"precision mediump float;                                \n"
	"uniform samplerExternalOES uTexSampler;                 \n"

	"varying vec2 vTexCoord;                                 \n"
//	"varying vec4 vColor;                                    \n"
	"void main()                                             \n"
	"{                                                       \n"
	"	vec4 temp = texture2D(uTexSampler, vTexCoord);    \n"
//	"		+ vColor;                                \n"
	"	gl_FragColor = vec4(temp.xyz, 1.0);              \n"
//	"	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);	 \n" //used for testing if everything works well when there is NO texture
	"}                                                       \n";

static GLfloat vVertices[] = {
		0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
		0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		};

/*static GLfloat vColors[] = {
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		};
*/

static GLfloat vTexVertices[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		};


static const char *
eglErrorString(EGLint code)
{
#define MYERRCODE(x) case x: return #x;
	switch (code) {
	MYERRCODE(EGL_SUCCESS)
	MYERRCODE(EGL_NOT_INITIALIZED)
	MYERRCODE(EGL_BAD_ACCESS)
	MYERRCODE(EGL_BAD_ALLOC)
	MYERRCODE(EGL_BAD_ATTRIBUTE)
	MYERRCODE(EGL_BAD_CONTEXT)
	MYERRCODE(EGL_BAD_CONFIG)
	MYERRCODE(EGL_BAD_CURRENT_SURFACE)
	MYERRCODE(EGL_BAD_DISPLAY)
	MYERRCODE(EGL_BAD_SURFACE)
	MYERRCODE(EGL_BAD_MATCH)
	MYERRCODE(EGL_BAD_PARAMETER)
	MYERRCODE(EGL_BAD_NATIVE_PIXMAP)
	MYERRCODE(EGL_BAD_NATIVE_WINDOW)
	MYERRCODE(EGL_CONTEXT_LOST)
	default:
		return "unknown";
	}
#undef MYERRCODE
}

static int
checkEglErrorState(void)
{
	EGLint code;

	code = eglGetError();
	if (code != EGL_SUCCESS) {
		printf("EGL error state: %s (0x%04lx)\n",
			eglErrorString(code), (long)code);
		return -1;
	}

	return 0;
}

static const char *
glErrorString(EGLint code)
{
#define MYERRCODE(x) case x: return #x;
	switch (code) {
	MYERRCODE(GL_NO_ERROR);
	MYERRCODE(GL_INVALID_ENUM);
	MYERRCODE(GL_INVALID_VALUE);
	MYERRCODE(GL_INVALID_OPERATION);
	MYERRCODE(GL_STACK_OVERFLOW_KHR);
	MYERRCODE(GL_STACK_UNDERFLOW_KHR);
	MYERRCODE(GL_OUT_OF_MEMORY);
	default:
		return "unknown";
	}
#undef MYERRCODE
}

static bool
checkEglExtension(const char *extensions, const char *extension)
{
	size_t extlen = strlen(extension);
	const char *end = extensions + strlen(extensions);

	while (extensions < end) {
		size_t n = 0;

		/* Skip whitespaces, if any */
		if (*extensions == ' ') {
			extensions++;
			continue;
		}

		n = strcspn(extensions, " ");

		/* Compare strings */
		if (n == extlen && strncmp(extension, extensions, n) == 0)
			return true; /* Found */

		extensions += n;
	}

	/* Not found */
	return false;
}


static int checkGlErrorState(void)
{
	EGLint code;

	code = glGetError();
	if (code != GL_NO_ERROR) {
		printf("GL error state: %s (0x%04lx)\n",
			glErrorString(code), (long)code);
		return -1;
	}

	return 0;
}

static unsigned int getDrmFormat(char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(DrmFormat); i++) {
		if (!strcmp(DrmFormat[i].name, name)) {
			return DrmFormat[i].drm_format;
		}
	}

	return DRM_FORMAT_ARGB8888;
}

void getTestPara(struct TestCase *test, int argc, char *argv[])
{
	int i = 0;

	while(i < argc) {
		if (!strcmp(argv[i], "-tw")) {
			if (argc > i + 1) {
				i += 1;
				test->tex_width = atoi(argv[i]);
				printf("Texture Width:%u\n", test->tex_width);
			}
		}

		if (!strcmp(argv[i], "-th")) {
			if (argc > i + 1) {
				i += 1;
				test->tex_height = atoi(argv[i]);
				printf("Texture Height:%u\n", test->tex_height);
			}
		}

		if (!strcmp(argv[i], "-vw")) {
			if (argc > i + 1) {
				i += 1;
				test->view_width = atoi(argv[i]);
				printf("View Width:%u\n", test->view_width);
			}
		}

		if (!strcmp(argv[i], "-vh")) {
			if (argc > i + 1) {
				i += 1;
				test->view_height = atoi(argv[i]);
				printf("View Height:%u\n", test->view_height);
			}
		}

		if (!strcmp(argv[i], "-tf")) {
			if (argc > i+1) {
				i += 1;
				test->TexFormat = getDrmFormat(argv[i]);
				printf("Textrure Format:%s  0x%x\n", argv[i], test->TexFormat);
			}
		}


		if (!strcmp(argv[i], "-of")) {
			if (argc > i+1) {
				i += 1;
				test->OutFormat = getDrmFormat(argv[i]);
				printf("Out Format:%s  0x%x\n", argv[i], test->OutFormat);
			}
		}

		if (!strcmp(argv[i], "-tpath")) {
			if (argc > i+1) {
				i += 1;
				test->TexPath[0] = '\0';
				sprintf(test->TexPath, "%s", argv[i]);
				printf("texture path:%s\n", test->TexPath);
			}
		}

		if (!strcmp(argv[i], "-opath")) {
			if (argc > i+1) {
				i += 1;
				test->OutPath[0] = '\0';
				sprintf(test->OutPath, "%s", argv[i]);
				printf("output path:%s\n", test->OutPath);
			}
		}

		i++;
	}

	printf("\n\n\n");
}

unsigned int getImageSize(unsigned int format,
	unsigned int width, unsigned int height)
{
	switch (format) {
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
		return width * height * 4;

	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV12:
		return width * height * 3 / 2;
	}

	return 0;
}

void Redraw(struct TestCase *test)
{
	glViewport(0, 0, test->view_width, test->view_height);

	//set background color, NOTE! these setting must be at the time after create FBO
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#ifdef ON_SCREEN
	eglSwapInterval(test->renderer.egl_display, 1);
	eglSwapBuffers(test->renderer.egl_display,
			test->renderer.egl_surface);
#endif

	//rendering by a blocking way(for test)
	glFinish();
}



int ion_open(void)
{
	int ion_fd;

	ion_fd = open("/dev/ion", O_RDWR, 0);
	if (ion_fd <= 0) {
		printf("open ion failed!\n");
		return -1;
	}

	return ion_fd;
}

void ion_close(int ion_fd)
{
	if (ion_fd > 0)
		close(ion_fd);
}

static int dma_alloc(int ion_fd, struct ion_mem *mem, int size)
{
	int ret;
	void *addrmap = NULL;

	printf("dma alloc: ionfd:%d  size:%d\n", ion_fd, size);

	/*For the performance reason, we are suggested to use Manual Cache*/
	struct ion_allocation_data sAllocInfo = {

		.len		= size,
		.align		= 4096,
		//.heap_id_mask	= ION_HEAP_CARVEOUT_MASK,
		.heap_id_mask = ION_HEAP_TYPE_DMA_MASK,
		//.flags		= 0,
		.flags                = ION_FLAG_CACHED, //Auto Cache, for Test
		//.flags		= ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC, //Manual Cache
		.handle     = 0,
	};
	ret = ioctl(ion_fd, ION_IOC_ALLOC, &sAllocInfo);
	if (ret < 0) {
		printf("alloc ion err.\n");
		close(ion_fd);
		return  -1;
	}

	struct ion_fd_data data = {
		.handle = sAllocInfo.handle,
	};

	ret = ioctl(ion_fd, ION_IOC_SHARE, &data);
	//ret = ioctl(ion_fd, ION_IOC_MAP, &data);
	if (ret < 0)
		return ret;

	if (data.fd < 0) {
		printf("ION_IOC_MAP failed!\n");
		ioctl(ion_fd, ION_IOC_FREE, &sAllocInfo);
		return -1;
	}

	/* mmap to user */
	addrmap = mmap(NULL, sAllocInfo.len, PROT_READ | PROT_WRITE, MAP_SHARED, data.fd, 0);
	if(MAP_FAILED == addrmap) {
		printf("mmap err!\n");
		addrmap = NULL;
		ioctl(ion_fd, ION_IOC_FREE, &sAllocInfo);
		return -1;
	}
	memset(addrmap, 0, sAllocInfo.len);

	mem->phy = 0;
	mem->virt = (unsigned char *)addrmap;
	mem->size = size;
	mem->dmafd = data.fd;
	mem->handle = sAllocInfo.handle;

	return data.fd;
}

void ion_free(int ion_fd, struct ion_mem *mem)
{
	struct ion_handle_data handle_data;

	if (mem->virt) {
		munmap((void*)mem->virt, mem->size);
		mem->virt = 0;
	}

	if (mem->dmafd > 0) {
		close(mem->dmafd);
		mem->dmafd = -1;
	}

	if (mem->handle > 0) {
		handle_data.handle = mem->handle;
		ioctl(ion_fd, ION_IOC_FREE, &handle_data);
		mem->handle = 0;
	}
}

int load_texture(struct TestCase *test, struct ion_mem *mem)
{
	void *addr_file;
	int dma_fd;
	int file_fd;
	unsigned int size = getImageSize(test->TexFormat,
		test->tex_width, test->tex_height);

	dma_fd = dma_alloc(test->ion_fd, mem, size);
	if (dma_fd < 0) {
		printf("dma_alloc for texture failed!\n");
		return -1;
	}
	printf("succed to alloc 0x%x size of dmabuffer for texture\n", size);

	//load textue
	file_fd = open(test->TexPath, O_RDWR);
	if (file_fd < 0) {
		printf("open %s err.\n", test->TexPath);
		return -1;
	}
	addr_file = (void *)mmap((void *)0, size,
				 PROT_READ | PROT_WRITE, MAP_SHARED,
				 file_fd, 0);

	//copy texture into dma buffer
	memcpy(mem->virt, addr_file , size);

	//sync for dma gpu， here memorys will be operated by CPU，so we need to flush cache
/*	struct ion_fd_data data2 = {
		.fd = dma_fd,
	};

	//finishing loading
	ioctl(ion_fd, ION_IOC_SYNC, &data2);
*/
	munmap(addr_file ,size);

	printf("succeed to load texuture!\n");

	return dma_fd;
}

static EGLImageKHR creatEglImage(EGLDisplay dpy, int dma_fd,
		unsigned int width, unsigned int height, unsigned int format)
{
	int atti = 0;
	EGLint attribs0[30];

	//set the image's size
	attribs0[atti++] = EGL_WIDTH;
	attribs0[atti++] = width;
	attribs0[atti++] = EGL_HEIGHT;
	attribs0[atti++] = height;

	//set pixel format
	attribs0[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
	attribs0[atti++] = format;

	switch (format) {
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
		attribs0[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
		attribs0[atti++] = dma_fd;
		attribs0[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
		attribs0[atti++] = 0;
		attribs0[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
		attribs0[atti++] = width * 4;
		break;
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV12:
		//set buffer fd, offset, pitch for y component
		attribs0[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
		attribs0[atti++] = dma_fd;
		attribs0[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
		attribs0[atti++] = 0;
		attribs0[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
		attribs0[atti++] = width;

		//set buffer fd, offset ,pitch
		attribs0[atti++] = EGL_DMA_BUF_PLANE1_FD_EXT;
		attribs0[atti++] = dma_fd;
		attribs0[atti++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
		attribs0[atti++] = width * height;
		attribs0[atti++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
		attribs0[atti++] = width;
		break;
	default:
		printf("format:%d NOT support\n", format);
	};

	//set color space and color range
	attribs0[atti++] = EGL_YUV_COLOR_SPACE_HINT_EXT;
	attribs0[atti++] = EGL_ITU_REC709_EXT;
	attribs0[atti++] = EGL_SAMPLE_RANGE_HINT_EXT;
	attribs0[atti++] = EGL_YUV_FULL_RANGE_EXT;

	attribs0[atti++] = EGL_NONE;

	//Notice the target: EGL_LINUX_DMA_BUF_EXT
	return eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
				0, attribs0);
}

static int setupTexture(EGLDisplay dpy, int dmafd,
		unsigned int width, unsigned int height, unsigned int format)
{
	GLuint Tex;
	EGLImageKHR img0;

	glActiveTexture(GL_TEXTURE0);

	img0 = creatEglImage(dpy, dmafd, width, height, format);
	if (img0 == EGL_NO_IMAGE_KHR) {
		checkEglErrorState();
		return -1;
	}

	//create gl texture and refered to EGL Image
	glGenTextures(1, &Tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, Tex);

	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)img0);
	if (checkEglErrorState())
		return -1;
	if (checkGlErrorState())
		return -1;

	printf("secceed to setupTexture!\n");

	return Tex;
}

int createFBO(struct TestCase *test, EGLDisplay dpy, struct ion_mem *mem,
		GLuint *Fbo, GLuint *tex)
{
	EGLImageKHR img0;
	GLuint Tex;
	unsigned int size = getImageSize(test->OutFormat,
		test->view_width, test->view_height);

	int dma_fd = dma_alloc(test->ion_fd, mem, size);
	if (dma_fd < 0) {
		printf("dma_alloc for fbo failed!\n");
		return -1;
	}
	printf("succeed to alloc 0x%x size of dmabuffer for fbo\n", size);

	img0 = creatEglImage(dpy, dma_fd,
			     test->view_width, test->view_height,
			     test->OutFormat);
	if (img0 == EGL_NO_IMAGE_KHR) {
		checkEglErrorState();
		return -1;
	}


	glGenTextures(1, &Tex);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, Tex);

	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)img0);
	if (checkEglErrorState())
		return -1;
	if (checkGlErrorState())
		return -1;

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	//The texture target MUST be GL_TEXTURE_EXTERNAL_OES, NOT GL_TEXTURE_2D
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_EXTERNAL_OES, Tex, 0);
	uint32_t glStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (glStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("glCheckFramebufferStatusOES error 0x%x", glStatus);
		return -1;
	}

	if (checkGlErrorState())
		return -1;

	glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

	*Fbo = fbo;
	*tex = Tex;

	printf("succeed to create FBO\n");

	return dma_fd;
}

void outputRenderResult(int ion_fd, struct ion_mem *mem,
			char *path, unsigned int size)
{
	FILE *fp;

	fp=fopen(path, "w+");
	fwrite(mem->virt, 1, size, fp);


	//sync for dma gpu, ion do the manual cache
/*	struct ion_fd_data data2 = {
		.fd = mem->dmafd,
	};

	//finishing loading
	ioctl(ion_fd, ION_IOC_SYNC, &data2);
*/
	fclose(fp);
}

int sendForDisplay(int ion_fd, struct ion_mem *mem)
{
	struct fb_var_screeninfo vinfo;
	int fd;
	unsigned char *fbbuf;

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		printf("failed to open dev/fb0\n");
		return 0;
	}

	memset(&vinfo, 0, sizeof(vinfo));
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("failed to get fb info\n");
		return 0;
	}

	if ((fbbuf = mmap(0, mem->size,
		PROT_READ | PROT_WRITE, MAP_SHARED,
		fd, 0)) == (void*) -1) {
		printf("fbdev map video error\n");
		return -1;
	}

//Note: This is only dma buffer transmission. so do NOT need to flush cache
	memcpy(fbbuf, mem->virt, mem->size);

	close(fd);
	return 0;
}

static EGLint const config_attribute_list[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_BUFFER_SIZE, 32,

	EGL_STENCIL_SIZE, 0,
	EGL_DEPTH_SIZE, 0,

	EGL_SAMPLES, 4,

	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,

#ifdef ON_SCREEN
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
#else
	EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
#endif
	EGL_NONE
};

#ifdef ON_SCREEN
static EGLint window_attribute_list[] = {
	EGL_NONE
};
#endif

static const EGLint context_attribute_list[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};
int initEGL(struct TestCase *test)
{
	EGLConfig config;
	EGLint num_config;
	const char *extensions;

	struct Renderer *renderer = &test->renderer;

	renderer->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (renderer->egl_display == EGL_NO_DISPLAY) {
		fprintf(stderr, "Error: No display found!\n");
		return -1;
	}

	if (!eglInitialize(renderer->egl_display,
		&renderer->egl_major, &renderer->egl_minor)) {
		fprintf(stderr, "Error: eglInitialise failed!\n");
		return -1;
	}

	printf("EGL Version: \"%s\"\n",
	       eglQueryString(renderer->egl_display, EGL_VERSION));
	printf("EGL Vendor: \"%s\"\n",
	       eglQueryString(renderer->egl_display, EGL_VENDOR));
	extensions = eglQueryString(renderer->egl_display, EGL_EXTENSIONS);
	printf("EGL Extensions: \"%s\"\n", extensions);
	printf("\n\n\n");

	if (!checkEglExtension(extensions, "EGL_EXT_image_dma_buf_import")) {
		printf("egl extension image can NOT be imported by dma buffer\n");
		return -1;
	}

	if (!eglBindAPI (EGL_OPENGL_ES_API)) {
		printf("failed to bind api EGL_OPENGL_ES_API\n");
		return -1;
	}

	eglChooseConfig(renderer->egl_display, config_attribute_list, &config, 1,
			&num_config);

	renderer->egl_context = eglCreateContext(renderer->egl_display, config,
			EGL_NO_CONTEXT, context_attribute_list);
	if (renderer->egl_context == EGL_NO_CONTEXT) {
		checkEglErrorState();
		return -1;
	}

#ifdef ON_SCREEN
	GLint width, height;

	//obtain an surface(buffer) that can be display diractly
	native_window.width = test->view_width;
	native_window.height = test->view_height;
	renderer->egl_surface = eglCreateWindowSurface(
					    renderer->egl_display, config,
					    (NativeWindowType)&native_window,
					    window_attribute_list);
	if (renderer->egl_surface == EGL_NO_SURFACE) {
		checkEglErrorState();
		return -1;
	}

	if (!eglQuerySurface(renderer->egl_display, renderer->egl_surface,
		EGL_WIDTH, &width) ||
	    !eglQuerySurface(renderer->egl_display, renderer->egl_surface,
	    	EGL_HEIGHT, &height)) {
		checkEglErrorState();
		return -1;
	}
	printf("Surface size: %d x %d\n", width, height);
#else
	renderer->egl_surface = eglCreatePbufferSurface(
				renderer->egl_display, config, NULL);
	if (renderer->egl_surface == EGL_NO_SURFACE) {
		checkEglErrorState();
		return -1;
	}
#endif



	if (!eglMakeCurrent(renderer->egl_display, renderer->egl_surface,
			renderer->egl_surface, renderer->egl_context)) {
		checkEglErrorState();
		return -1;
	}

	eglSwapInterval(renderer->egl_display, 1);

//obtain EGL extension API address
/*	eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)
			    eglGetProcAddress("eglCreateImageKHR");
	glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
			    eglGetProcAddress("glEGLImageTargetTexture2DOES");
	eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC)
			    eglGetProcAddress("eglCreateSyncKHR");
	eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC)
			    eglGetProcAddress("eglDestroySyncKHR");
	eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC)
			    eglGetProcAddress("eglClientWaitSyncKHR");
*/

	return 0;
}

int createShader(struct Shader *shader, int type, const char *source)
{
	GLint ret;

	shader->id = glCreateShader(type);
	if (!shader->id) {
		checkGlErrorState();
		return -1;
	}

	glShaderSource(shader->id, 1, &source, NULL);
	glCompileShader(shader->id);

	glGetShaderiv(shader->id, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		fprintf(stderr, "Error: shader compilation failed, type:%d!\n",
						type);
		glGetShaderiv(shader->id, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(shader->id, ret, NULL, log);
			fprintf(stderr, "%s", log);
		}
		return -1;
	}

	return 0;
}

int initShader(struct Renderer *renderer,
			const char *vertex, const char *fragment)
{
	GLint ret;

	if (createShader(&renderer->vs, GL_VERTEX_SHADER, vertex) < 0) {
		printf("createShader vertex shader failed!\n");
		return 0;
	}

	if (createShader(&renderer->fs, GL_FRAGMENT_SHADER, fragment) < 0) {
		printf("createShader fragment shader failed!\n");
		return 0;
	}

	renderer->shader_program = glCreateProgram();
	if (!renderer->shader_program) {
		fprintf(stderr, "Error: failed to create shader_program!\n");
		return -1;
	}

	glAttachShader(renderer->shader_program, renderer->vs.id);
	glAttachShader(renderer->shader_program, renderer->fs.id);

	glBindAttribLocation(renderer->shader_program, 0, "aPosition");
	glBindAttribLocation(renderer->shader_program, 1, "aTexCoord");
	//glBindAttribLocation(renderer->shader_program, 2, "aColor"); //Used for testing
	glLinkProgram(renderer->shader_program);

	glGetProgramiv(renderer->shader_program, GL_LINK_STATUS, &ret);
	if (!ret) {
		char *log;

		fprintf(stderr, "Error: shader_program linking failed!\n");
		glGetProgramiv(renderer->shader_program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetProgramInfoLog(renderer->shader_program, ret, NULL, log);
			fprintf(stderr, "%s", log);
		}
		return -1;
	}
	glUseProgram(renderer->shader_program);

	return 0;
}


char *Nv21In_ArgbOut[] = {
	"-tw",    "1280",			//Textrue width
	"-th",    "720",			//Texture height
	"-tf",    "nv21",			//Texture pixel format
	"-tpath", "./pic_bin/1280_720_nv21.yuv",//path of textrue file
	"-vw",    "1280",			//viewport width
	"-vh",    "800",			//viewport width
	"-of",    "argb8888",			//output pixel format
	"-opath", "./render_result.argb",	//path of output file that contains the rendering result
};

char *Nv12In_ArgbOut[] = {
	"-tw",    "1280",
	"-th",    "720",
	"-tf",    "nv12",
	"-tpath", "./pic_bin/1280_720_nv21.yuv",
	"-vw",    "1280",
	"-vh",    "800",
	"-of",    "argb8888",
	"-opath", "./render_result.argb",
};

char *ArgbIn_ArgbOut[] = {
	"-tw",    "1280",
	"-th",    "720",
	"-tf",    "rgba8888",
	"-tpath", "./pic_bin/rgba8888_1280_720.bin",
	"-vw",    "1280",
	"-vh",    "800",
	"-of",    "argb8888",
	"-opath", "./render_result.argb",
};

struct TestCase test;
int
main(int argc, char *argv[])
{
	int ret = 0;

	struct Renderer *renderer = &test.renderer;

	static GLint vTexSamplerHandler;

//Parse test parameters
	//if (argc <= 0) {
	//	printf("WARN: please intput para!\n");
	//	return -1;
	//}

	//getTestPara(&test, argc, argv);
	getTestPara(&test, ARRAY_SIZE(Nv21In_ArgbOut), Nv21In_ArgbOut);
	printf("render start!\n\n\n");

//Init ION
	test.ion_fd = ion_open();
	if (test.ion_fd < 0)
		return -1;

//Init EGL
	if (initEGL(&test) < 0) {
		printf("eglInitial failed!\n");
		return -1;
	}

	printf("GL Vendor: \"%s\"\n", glGetString(GL_VENDOR));
	printf("GL Renderer: \"%s\"\n", glGetString(GL_RENDERER));
	printf("GL Version: \"%s\"\n", glGetString(GL_VERSION));
	printf("GL Extensions: \"%s\"\n", glGetString(GL_EXTENSIONS));
	printf("\n\n\n");

//init shader: create, compile and link shader program
	if (initShader(renderer, vertex_shader_source,
			fragment_shader_source) < 0) {
		printf("initShader failed\n");
		return -1;
	}
	glUseProgram(renderer->shader_program);

//Set all of the vertex information
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, vTexVertices);
	glEnableVertexAttribArray(1);

	//Only for test
//	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, vColors);
//	glEnableVertexAttribArray(2);


//Create the GL texure and load the content of texture source into GL texture
	ret = load_texture(&test, &renderer->TexMem);
	if (ret < 0) {
		printf("load texture failed!\n");
		return -1;
	}
	renderer->Tex = setupTexture(
		renderer->egl_display, renderer->TexMem.dmafd,
		test.tex_width, test.tex_height, test.TexFormat);

#ifndef ON_SCREEN
//Create the FBO and attach an texture to its color buffer
	ret = createFBO(&test, renderer->egl_display, &renderer->FboMem,
			&renderer->fbo, &renderer->FboColorBufferTex);
	if (ret < 0) {
		printf("create FBO failed\n");
		return -1;
	}
#endif

//Set uniform value
	//Note: before setting uniform, we HAVE TO  active the shader program
	glUseProgram(renderer->shader_program);
	vTexSamplerHandler = glGetUniformLocation(renderer->shader_program, "uTexSampler");
	glUniform1i(vTexSamplerHandler, 0);

//Start drawing
	//NOTE: before starting drawing, we HAVE TO bind the Texure, Buffer, Framebuffer.
	//if we don't do so, errors always appear when using FBO.
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, renderer->Tex);
	glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);

	Redraw(&test);

#ifndef ON_SCREEN
//Output the result
	//NOTE: when using FBO, the rendering result that output always upside down,
	//so you have to do a 180 degree rotate, using G2D or doing next rendering by gpu
	sendForDisplay(test.ion_fd, &renderer->FboMem);  //display the rendering result directly, used fot testing
	outputRenderResult(test.ion_fd, &renderer->FboMem, //output the result to a file path, just for testing
		test.OutPath, renderer->FboMem.size);
#endif
	return 0;
}
