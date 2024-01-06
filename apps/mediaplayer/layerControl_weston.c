/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : layerControl_de.cpp
* Description : display weston
* History :
*/

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <AwPool.h>
#include <CdxQueue.h>

#include <cdx_config.h>
#include <cdx_log.h>
#include <CdxTime.h>
#include <layerControl.h>
#include <memoryAdapter.h>
#include <CdxIon.h>
#include <iniparserapi.h>

#include <xf86drm.h>
#include <i915_drm.h>
#include <drm_fourcc.h>
#include <wayland-client.h>
#include <libweston-5/zalloc.h>
#include <protocol/xdg-shell-unstable-v6-client-protocol.h>
#include <protocol/fullscreen-shell-unstable-v1-client-protocol.h>
#include <protocol/linux-dmabuf-unstable-v1-client-protocol.h>

#include <protocol/viewporter-client-protocol.h>
#include <protocol/ivi-application-client-protocol.h>
#define IVI_SURFACE_ID 9000

struct display
{
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor *compositor;
    struct zxdg_shell_v6 *shell;
    struct zwp_fullscreen_shell_v1 *fshell;
    struct zwp_linux_dmabuf_v1 *dmabuf;
    struct ivi_application *ivi_application;

    //struct wl_seat *seat;
    //struct wp_viewporter *viewporter;

    int xrgb8888_format_found;
}display;

enum eBufferOwner
{
    OWN_BY_LAYER,
    OWN_BY_DECODER,
    OWN_BY_WESTON
};

typedef struct WestonBufferS
{
    enum eBufferOwner owner;
    VideoPicture pPicture;
    CdxQueueT*   return_queue;
    CdxQueueT*   display_queue;

    int dmabuf_fd;
    //int drm_fd;
    int bpp;
    unsigned long stride;
    //struct drm_mode_create_dumb drm_buf;
    struct wl_buffer *wl_buffer;

} WestonBufferT;

struct window
{
    struct display *display;
    struct wl_surface *surface;
    struct zxdg_surface_v6 *xdg_surface;
    struct zxdg_toplevel_v6 *xdg_toplevel;
    struct buffer *prev_buffer;
    struct wl_callback *callback;
    struct ivi_surface *ivi_surface;

    int	 crop_x;				//display crop x
	int	 crop_y;				//display crop y
    int  crop_width;			//display crop width
    int  crop_height;			//display crop height

    int width;
    int height;
    enum EPIXELFORMAT pixel_format;
    int    buffer_number;

    int    init_buffer_flag;
    WestonBufferT* weston_buffers;

    AwPoolT             *pool;
    CdxQueueT*   return_queue;
    CdxQueueT*   display_queue;

    int wait_for_configure;
    int *pbQuit;
    uint32_t  preFrameTime;
    cdx_int64 systemGrapeTime;

    int   display_frame_number;
    cdx_int64 display_start_time;
}window;

typedef struct LayerContext
{
    LayerCtrl            base;

    void*                pUserData;

    int                  bLayerShowed;

    pthread_t            render_thread_id;

    struct display      *display;
    struct window       *window;

    int *pbQuit;

    int nUnFreeBufferNum;
} LayerContext;

static const struct wl_callback_listener frame_listener;
static void redraw(void *data, struct wl_callback *callback, uint32_t time);

static void destroy_window(LayerContext* lc)
{
    logd("destroy window");
    int i;
	if(!lc)
	{
		printf("destroy_window fail, input lc=null\n");
		return ;
	}

	if(!lc->window){
		printf("destroy_window fail, lc->window null\n");
		return ;
	}

    if (lc->window->callback)
        wl_callback_destroy(lc->window->callback);

    if (lc->window->xdg_toplevel)
        zxdg_toplevel_v6_destroy(lc->window->xdg_toplevel);
    if (lc->window->xdg_surface)
        zxdg_surface_v6_destroy(lc->window->xdg_surface);
//	if (lc->window->viewport)
//		wp_viewport_destroy(lc->window->viewport);
	if (lc->window->ivi_surface)
		ivi_surface_destroy(lc->window->ivi_surface);
    wl_surface_destroy(lc->window->surface);
    //drm_shutdown(lc->window);

    free(lc->window->weston_buffers);
    free(lc->window);
}

static void destroy_display(struct display *display)
{
    logd("destroy display");
    if (display->dmabuf)
        zwp_linux_dmabuf_v1_destroy(display->dmabuf);

    if (display->shell)
        zxdg_shell_v6_destroy(display->shell);

    if (display->fshell)
        zwp_fullscreen_shell_v1_release(display->fshell);

    if (display->compositor)
        wl_compositor_destroy(display->compositor);
//	if (display->viewporter)
//		wp_viewporter_destroy(display->viewporter);
//	if (display->seat)
//		wl_seat_destroy(display->seat);
	if (display->ivi_application)
		ivi_application_destroy(display->ivi_application);

    wl_registry_destroy(display->registry);
    wl_display_flush(display->display);
    wl_display_disconnect(display->display);
    free(display);
}

static void xdg_surface_handle_configure(void *data, struct zxdg_surface_v6 *surface,
                             uint32_t serial)
{
    struct window* window = data;

    zxdg_surface_v6_ack_configure(surface, serial);

    if (window->wait_for_configure)
        redraw(window, NULL, 0);
    window->wait_for_configure = false;
}

static const struct zxdg_surface_v6_listener xdg_surface_listener =
{
    xdg_surface_handle_configure,
};

static int __LayerReset(LayerCtrl* l)
{
    LayerContext* lc;
    int i;

    logd("LayerInit.");
    lc = (LayerContext*)l;

    for(i=0; i<lc->window->buffer_number; i++)
    {
        if(lc->window->weston_buffers[i].pPicture.pData0)
        {
            CdxIonPfree(lc->window->weston_buffers[i].pPicture.pData0);
            wl_buffer_destroy(lc->window->weston_buffers[i].wl_buffer);
            lc->window->weston_buffers[i].wl_buffer = NULL;
            close(lc->window->weston_buffers[i].dmabuf_fd);

            memset(&lc->window->weston_buffers[i], 0, sizeof(WestonBufferT));
            lc->nUnFreeBufferNum --;
        }
    }

    return 0;
}

static void __LayerRelease(LayerCtrl* l)
{
    LayerContext* lc;
    int i;
    lc = (LayerContext*)l;
    logd("Layer release");

    for(i=0; i<lc->window->buffer_number; i++)
    {
        if(lc->window->weston_buffers[i].pPicture.pData0)
        {
            CdxIonPfree(lc->window->weston_buffers[i].pPicture.pData0);
            wl_buffer_destroy(lc->window->weston_buffers[i].wl_buffer);
            lc->window->weston_buffers[i].wl_buffer = NULL;
            close(lc->window->weston_buffers[i].dmabuf_fd);

            memset(&lc->window->weston_buffers[i], 0, sizeof(WestonBufferT));
            lc->nUnFreeBufferNum --;
        }
    }

    lc->window->init_buffer_flag = 0;

    while(CdxQueueEmpty(lc->window->return_queue)!= true)
    {
        CdxQueuePop(lc->window->return_queue);
        logd("** CdxQueuePop ");
    }

    while(CdxQueueEmpty(lc->window->display_queue)!= true)
    {
        CdxQueuePop(lc->window->display_queue);
    }

    logd("  Layer release end");
    return ;
}

static void __LayerDestroy(LayerCtrl* l)
{
    LayerContext* lc;

    logd("__LayerDestroy");
	if(!l)
	{
		printf("__LayerDestroy fail, input null\n");
		return ;
	}

    lc = (LayerContext*)l;

    if(lc->nUnFreeBufferNum > 0)
    {
        logd("=== unrelease buffer : %d", lc->nUnFreeBufferNum);
    }

    CdxQueueDestroy(lc->window->display_queue);
    CdxQueueDestroy(lc->window->return_queue);

    destroy_window(lc);
    CdxIonClose();
    destroy_display(lc->display);

    free(lc);
}

static int __LayerSetDisplayBufferSize(LayerCtrl* l, int nWidth, int nHeight)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

    logd("__LayerSetDisplayBufferSize lc %p",lc);

    lc->window->width  = nWidth;
    lc->window->height = nHeight;

    return 0;
}

static int __LayerSetDisplayRegion(LayerCtrl* l, int nLeftOff, int nTopOff,
                                   int nDisplayWidth, int nDisplayHeight)
{
    LayerContext* lc;
    lc = (LayerContext*)l;
    logd("Layer set display region, leftOffset = %d, topOffset = %d, \
            displayWidth = %d, displayHeight = %d",
          nLeftOff, nTopOff, nDisplayWidth, nDisplayHeight);

    if(nDisplayWidth == 0 && nDisplayHeight == 0)
    {
        return -1;
    }

    lc->window->crop_x = nLeftOff;
    lc->window->crop_y = nTopOff;
    lc->window->crop_width = nDisplayWidth;
    lc->window->crop_height = nDisplayHeight;

    return 0;
}

static int __LayerSetDisplayPixelFormat(LayerCtrl* l, enum EPIXELFORMAT ePixelFormat)
{
    LayerContext* lc;

    lc = (LayerContext*)l;
    logd("Layer set expected pixel format, format = %d", (int)ePixelFormat);

    if(ePixelFormat == PIXEL_FORMAT_NV12 ||
            ePixelFormat == PIXEL_FORMAT_NV21 ||
            ePixelFormat == PIXEL_FORMAT_YV12)           //* add new pixel formats supported by gpu here.
    {
        lc->window->pixel_format = ePixelFormat;
    }
    else
    {
        logd("receive pixel format is %d, not match.", ePixelFormat);
        return -1;
    }

    return 0;
}

static int __LayerSetBufferTimeStamp(LayerCtrl* l, int64_t nPtsAbs)
{

    return 0;
}

static int __LayerCtrlShowVideo(LayerCtrl* l)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

    lc->bLayerShowed = 1;

    return 0;
}

static int __LayerCtrlHideVideo(LayerCtrl* l)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

    lc->bLayerShowed = 0;

    return 0;
}

static int __LayerCtrlIsVideoShow(LayerCtrl* l)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

    return lc->bLayerShowed;
}

static int  __LayerCtrlHoldLastPicture(LayerCtrl* l, int bHold)
{
    logd("LayerCtrlHoldLastPicture, bHold = %d", bHold);

    return 0;
}

static void buffer_return_from_weston(void *data, struct wl_buffer *buffer)
{
    logv("release buffer!");
    WestonBufferT *my_buffer = data;

    logv("Push buffer into return queue %p", &my_buffer->pPicture);
    int ret = CdxQueuePush(my_buffer->return_queue,my_buffer);
    if (ret != 0)
    {
        CDX_LOGE("Push packet into return queue failure...");
        return ;
    }
    my_buffer->owner = OWN_BY_LAYER;
}

static const struct wl_buffer_listener buffer_listener =
{
    buffer_return_from_weston
};


static void create_succeeded(void *data,
                 struct zwp_linux_buffer_params_v1 *params,
                 struct wl_buffer *new_buffer)
{
    logv("create_succeeded");
    WestonBufferT *buffer = data;

    buffer->wl_buffer = new_buffer;
    wl_buffer_add_listener(buffer->wl_buffer, &buffer_listener, buffer);

    zwp_linux_buffer_params_v1_destroy(params);
}

static void create_failed(void *data, struct zwp_linux_buffer_params_v1 *params)
{
    WestonBufferT *my_buffer = data;

    logv("Push buffer into return queue %p",&my_buffer->pPicture);
	logd("lhg: create_failed CdxQueuePush time:%lld", CdxMonoTimeUs());
    int ret = CdxQueuePush(my_buffer->return_queue,my_buffer);
    if (ret != 0)
    {
        CDX_LOGE("Push packet into return queue failure...");
        return ;
    }
    my_buffer->owner = OWN_BY_LAYER;

    zwp_linux_buffer_params_v1_destroy(params);
    logd("Error: zwp_linux_buffer_params.create failed.");
}

static const struct zwp_linux_buffer_params_v1_listener params_listener =
{
    create_succeeded,
    create_failed
};

//* set usage, scaling_mode, pixelFormat, buffers_geometry, buffers_count, crop
static int setLayerBuffer(LayerContext* lc)
{
    logd("setLayerBuffer: PixelFormat(%d), nW(%d), nH(%d), leftoff(%d), topoff(%d)",
         lc->window->pixel_format,lc->window->width,
         lc->window->height,lc->window->crop_x,lc->window->crop_y);
    logd("setLayerBuffer: dispW(%d), dispH(%d), buffercount(%d)",
         lc->window->crop_width, lc->window->crop_height, lc->window->buffer_number);

    unsigned int nGpuBufWidth;
    unsigned int nGpuBufHeight;
    int i = 0;
    int dma_fd;

    char* pVirBuf;
    char* pPhyBuf;

    struct zwp_linux_buffer_params_v1 *params;
    uint64_t modifier;
    uint32_t flags;

    nGpuBufWidth  = lc->window->width;  //* restore nGpuBufWidth to mWidth;
    nGpuBufHeight = lc->window->height;

    if(lc->window->buffer_number <= 0)
    {
        loge("error: the lc->nGpuBufferCount[%d] is invalid!",lc->window->buffer_number);
        return -1;
    }

    lc->window->weston_buffers = (WestonBufferT*)malloc(lc->window->buffer_number * sizeof(WestonBufferT));

    for(i=0; i<lc->window->buffer_number; i++)
    {
        //logd("alloc in buff index %d!",i);
        pVirBuf = (char*)CdxIonPalloc(nGpuBufWidth * nGpuBufHeight * 3/2);
        pPhyBuf = (char*)CdxIonVir2Phy(pVirBuf);
        dma_fd = CdxIonPhy2ShareFd(pPhyBuf);

        lc->window->weston_buffers[i].pPicture.nWidth = lc->window->width;
        lc->window->weston_buffers[i].pPicture.nHeight = lc->window->height;
        lc->window->weston_buffers[i].pPicture.nLineStride = lc->window->width;
        lc->window->weston_buffers[i].pPicture.pData0 = pVirBuf;
        lc->window->weston_buffers[i].pPicture.pData1 = pVirBuf+(lc->window->width * lc->window->height);
        lc->window->weston_buffers[i].pPicture.pData2 = \
             lc->window->weston_buffers[i].pPicture.pData1 + (lc->window->width * lc->window->height)/4;
        lc->window->weston_buffers[i].pPicture.phyYBufAddr = (uintptr_t)pPhyBuf;
        lc->window->weston_buffers[i].pPicture.phyCBufAddr = \
                (uintptr_t)pPhyBuf + (lc->window->width * lc->window->height);
        lc->window->weston_buffers[i].pPicture.nBufId = i;
        lc->window->weston_buffers[i].pPicture.nBufFd = dma_fd;
        lc->window->weston_buffers[i].pPicture.ePixelFormat = lc->window->pixel_format;
        lc->window->weston_buffers[i].display_queue = lc->window->display_queue;
        lc->window->weston_buffers[i].return_queue = lc->window->return_queue;

        logd("==== init i: %d, pVirBuf %p, pPhyBuf %p, dma_fd  %d", i, pVirBuf, pPhyBuf, dma_fd);

        lc->window->weston_buffers[i].owner = OWN_BY_LAYER;
        lc->nUnFreeBufferNum ++;

        modifier = 0;
        flags = 0;

        params = zwp_linux_dmabuf_v1_create_params(lc->display->dmabuf);
        zwp_linux_buffer_params_v1_add(params,
                                       dma_fd,
                                       0, /* plane_idx */
                                       0, /* offset */
                                       lc->window->width,
                                       modifier >> 32,
                                       modifier & 0xffffffff);
        zwp_linux_buffer_params_v1_add(params,
                                       dma_fd,
                                       1, /* plane_idx */
                                       lc->window->width * lc->window->height, /* offset */
                                       lc->window->width,
                                       modifier >> 32,
                                       modifier & 0xffffffff);
        zwp_linux_buffer_params_v1_add_listener(params, &params_listener, &lc->window->weston_buffers[i]);

        if (lc->window->pixel_format == PIXEL_FORMAT_NV21)
        {
            zwp_linux_buffer_params_v1_create(params,
                                              lc->window->width,
                                              lc->window->height,
                                              DRM_FORMAT_NV21,
                                              flags);
        }
        else if (lc->window->pixel_format == PIXEL_FORMAT_NV12)
        {
            zwp_linux_buffer_params_v1_create(params,
                                              lc->window->width,
                                              lc->window->height,
                                              DRM_FORMAT_NV12,
                                              flags);
        }
        else if (lc->window->pixel_format == PIXEL_FORMAT_YV12)
        {
            zwp_linux_buffer_params_v1_create(params,
                                              lc->window->width,
                                              lc->window->height,
                                              DRM_FORMAT_YVU420,
                                              flags);
        }
        else
        {
            loge("eDisplayPixelFormat(%d) not support!",lc->window->pixel_format);
            return -1;
        }
    }

    logd("===== block here 11111");
    // block until all pending request are processed by the server
    //wl_display_roundtrip(lc->display->display);
    logd("===== block end 11111");

    return 0;
}

static WestonBufferT *window_next_buffer(struct window *window,uint32_t time)
{
    WestonBufferT*     nodePtr;

	if(!window->display_queue)
	{
		printf("window_next_buffer displayQueue=null\n");
		return NULL;
	}

	logv("=== CdxQueueEmpty: %d", CdxQueueEmpty(window->display_queue));
    while (CdxQueueEmpty(window->display_queue))
    {
        if (*window->pbQuit)
            return NULL;
        usleep(1000);
    }

    nodePtr = CdxQueuePop(window->display_queue);
    if(!nodePtr)
	{
		printf("nodePtr=null\n");
		return NULL;
	}

    nodePtr->owner = OWN_BY_LAYER;

    return nodePtr;
}

static void redraw(void *data, struct wl_callback *callback, uint32_t time)
{
    logv("call redraw callback %p time %d",callback,time);
    struct window *window = data;
    WestonBufferT *buffer;

    cdx_int64 now = CdxMonoTimeUs();

    if(window->display_start_time == 0)
    {
        window->display_start_time = now;
    }

    if(window->display_frame_number > 60)
    {
        logd(" display fps: %f", (double)window->display_frame_number*1000000 / (now - window->display_start_time));
        window->display_frame_number = 0;
        window->display_start_time = now;
    }

    uint32_t pass = time - window->preFrameTime;
    logv("pass time %d",pass);

    window->systemGrapeTime = now;
    buffer = window_next_buffer(window,pass);

    if ( buffer == NULL )
    {
        if (!callback)
            logd("Failed to create the first buffer.");
        else
            logd("All buffers busy at redraw(). Server bug?");
        return;
    }

    /* XXX: would be nice to draw something that changes here... */
    //logd("buffer drm_fd %d",buffer->drm_fd);
/*
	if ( NULL != window->viewport )
	{
		wp_viewport_set_source(window->viewport,
		wl_fixed_from_int(window->crop_x),
		wl_fixed_from_int(window->crop_y),
		wl_fixed_from_int(window->crop_width),
		wl_fixed_from_int(window->crop_height));
	}

	if(!window->is_positioned)
	{
		if(window->xdg_toplevel)
		// set the serial as 0xFFFFFFFF and use this IF to initialize the window position
			zxdg_toplevel_v6_show_window_menu(window->xdg_toplevel,window->display->seat,0xFFFFFFFF,window->nLeftOff,window->nTopOff);

		else if(window->ivi_surface)
		{
			logd("ivi surface should be inited by controller");
		}
		else
			logd("The surface's position couldn't be initialized!");
		window->is_positioned = 1;
	}
    */

	wl_surface_attach(window->surface, buffer->wl_buffer, 0, 0);

/*
	if (window->viewport)
		wp_viewport_set_destination(window->viewport,
					    window->width,
					    window->height);
*/
    wl_surface_damage(window->surface, 0, 0, window->width, window->height);

    if (callback)
        wl_callback_destroy(callback);

    window->callback = wl_surface_frame(window->surface);
    wl_callback_add_listener(window->callback, &frame_listener, window);
    wl_surface_commit(window->surface);

    window->preFrameTime = time;

    window->display_frame_number ++;
}

static const struct wl_callback_listener frame_listener =
{
    redraw
};

static void* westonRenderThread(void *arg)
{
    LayerContext* lc = (LayerContext*)arg;
    int ret = 0;

    logd("weston render thread start!");

    if (!lc->window->wait_for_configure)
        redraw(lc->window, NULL, 0);

    logd("westonRenderThread start while");
    while (ret != -1)
    {
        if (*lc->pbQuit == 1)
        {
            logd("user quit!");
            break;
        }
        logv("get wl_display_dispatch!");
        ret = wl_display_dispatch(lc->display->display);
//        logd("get wl_display_dispatch ok!");
    }
    logd("westonRenderThread while end!");

    return NULL;
}

static int __LayerDequeueBuffer(LayerCtrl* l, VideoPicture** ppVideoPicture, int bInitFlag)
{
    int i;
    LayerContext* lc = (LayerContext*)l;
    WestonBufferT* nodePtr = NULL;
    VideoPicture* pPicture = NULL;

    if(lc->window->init_buffer_flag == 0)
    {
        setLayerBuffer(lc);
        lc->window->init_buffer_flag = 1;
    }

    if(lc->render_thread_id == 0)
        pthread_create(&lc->render_thread_id, NULL, westonRenderThread, (void*)lc);

    if(bInitFlag == 1)
    {
        for(i=0; i<lc->window->buffer_number; i++)
        {
            if(lc->window->weston_buffers[i].owner == OWN_BY_LAYER)
            {
                pPicture = &lc->window->weston_buffers[i].pPicture;
                lc->window->weston_buffers[i].owner = OWN_BY_DECODER;
                break;
            }
        }
    }
    else
    {
        while(CdxQueueEmpty(lc->window->return_queue) == true)
        {
            if(*lc->pbQuit == 1)
            {
                *ppVideoPicture = NULL;
                return -1;
            }
            usleep(1000);
        }

        if(CdxQueueEmpty(lc->window->return_queue) != true)
        {
            nodePtr = CdxQueuePop(lc->window->return_queue);
            pPicture = &(nodePtr->pPicture);
            logv("** dequeue pPicture (%p)",pPicture);
        }

        for(i=0; i<lc->window->buffer_number; i++)
        {
            if(lc->window->weston_buffers[i].pPicture.pData0 == pPicture->pData0)
            {
                lc->window->weston_buffers[i].owner = OWN_BY_DECODER;
            }
        }
    }

    *ppVideoPicture = pPicture;
    logv(" LayerDequeueBuffer pData0: %p, bufId: %d, nID:%d", pPicture->pData0, pPicture->nBufId, pPicture->nID);
    return 0;
}

static int __LayerQueueBuffer(LayerCtrl* l, VideoPicture* pBuf, int bValid)
{
    WestonBufferT* newNode;
    int ret;
    int i;
    LayerContext* lc = (LayerContext*)l;
    logv(" __LayerQueueBuffer pData0: %p, bufId: %d; nID: %d", pBuf->pData0, pBuf->nBufId, pBuf->nID);

    if(pBuf == NULL)
    {
        loge("not available buffer");
        return -1;
    }

    if(lc->window->init_buffer_flag == 0)
    {
        setLayerBuffer(lc);
        lc->window->init_buffer_flag = 1;
    }

    for(i=0; i<lc->window->buffer_number; i++)
    {
        if(lc->window->weston_buffers[i].pPicture.pData0 == pBuf->pData0)
        {
            newNode = &lc->window->weston_buffers[i];
            lc->window->weston_buffers[i].owner = OWN_BY_LAYER;

            if(bValid)
            {
                ret = CdxQueuePush(lc->window->display_queue, newNode);
                if(ret != 0)
                {
                    loge("Push buffer into display queue failure...");
                    return -1;
                }
            }
            else
            {
                ret = CdxQueuePush(lc->window->return_queue, newNode);
                if(ret != 0)
                {
                    loge("Push buffer into display queue failure...");
                    return -1;
                }
            }
            break;
        }
    }

    return 0;
}

#define GPU_BUFFER_NUM 32
static int __LayerSetDisplayBufferCount(LayerCtrl* l, int nBufferCount)
{
    LayerContext* lc;

    lc = (LayerContext*)l;

    logd("LayerSetBufferCount: count = %d",nBufferCount);

    lc->window->buffer_number = nBufferCount;

    if(lc->window->buffer_number > GPU_BUFFER_NUM)
        logd("buffer number is too large");

    return lc->window->buffer_number;
}

static int __LayerGetBufferNumHoldByGpu(LayerCtrl* l)
{
    (void)l;
    return GetConfigParamterInt("pic_4list_num", 3);
}

static int __LayerGetDisplayFPS(LayerCtrl* l)
{
    (void)l;
    return 60;
}

static void __LayerResetNativeWindow(LayerCtrl* l,void* pNativeWindow)
{
    logd("LayerResetNativeWindow : %p ",pNativeWindow);

    return ;
}

// used in the case ResetNativeWindow, no need in weston
static VideoPicture* __LayerGetBufferOwnedByGpu(LayerCtrl* l)
{

    return NULL;
}

static int __LayerSetVideoWithTwoStreamFlag(LayerCtrl* l, int bVideoWithTwoStreamFlag)
{
    return 0;
}

static int __LayerSetIsSoftDecoderFlag(LayerCtrl* l, int bIsSoftDecoderFlag)
{
    return 0;
}

//* Description: the picture buf is secure
static int __LayerSetSecure(LayerCtrl* l, int bSecure)
{
    logd("__LayerSetSecure, bSecure = %d", bSecure);
    //*TODO

    return 0;
}
static int __LayerControl(LayerCtrl* l, int cmd, void *para)
{
    LayerContext *lc = (LayerContext*)l;

    CDX_UNUSE(para);

    switch(cmd)
    {
        default:
            break;
    }
    return 0;
}

static int __LayerReleaseBuffer(LayerCtrl* l, VideoPicture* pPicture)
{
    logd("***LayerReleaseBuffer");
    LayerContext* lc;

    lc = (LayerContext*)l;

    CdxIonPfree(pPicture->pData0);
    lc->nUnFreeBufferNum --;
    return 0;
}

static LayerControlOpsT mLayerControlOps =
{
release:
    __LayerRelease                   ,

setSecureFlag:
    __LayerSetSecure                 ,
setDisplayBufferSize:
    __LayerSetDisplayBufferSize      ,
setDisplayBufferCount:
    __LayerSetDisplayBufferCount     ,
setDisplayRegion:
    __LayerSetDisplayRegion          ,
setDisplayPixelFormat:
    __LayerSetDisplayPixelFormat     ,
setVideoWithTwoStreamFlag:
    __LayerSetVideoWithTwoStreamFlag ,
setIsSoftDecoderFlag:
    __LayerSetIsSoftDecoderFlag      ,
setBufferTimeStamp:
    __LayerSetBufferTimeStamp        ,

resetNativeWindow :
    __LayerResetNativeWindow         ,
getBufferOwnedByGpu:
    __LayerGetBufferOwnedByGpu       ,
getDisplayFPS:
    __LayerGetDisplayFPS             ,
getBufferNumHoldByGpu:
    __LayerGetBufferNumHoldByGpu     ,

ctrlShowVideo :
    __LayerCtrlShowVideo             ,
ctrlHideVideo:
    __LayerCtrlHideVideo             ,
ctrlIsVideoShow:
    __LayerCtrlIsVideoShow           ,
ctrlHoldLastPicture :
    __LayerCtrlHoldLastPicture       ,

dequeueBuffer:
    __LayerDequeueBuffer             ,
queueBuffer:
    __LayerQueueBuffer               ,
releaseBuffer:
    __LayerReleaseBuffer             ,
reset:
    __LayerReset                     ,


destroy:
    __LayerDestroy,
control:                    __LayerControl

};



static void xdg_toplevel_handle_configure(void *data, struct zxdg_toplevel_v6 *toplevel,
                              int32_t width, int32_t height,
                              struct wl_array *states)
{
}

static void xdg_toplevel_handle_close(void *data, struct zxdg_toplevel_v6 *xdg_toplevel)
{
    struct window *window = data;
    *window->pbQuit = 1;
}

static const struct zxdg_toplevel_v6_listener xdg_toplevel_listener =
{
    xdg_toplevel_handle_configure,
    xdg_toplevel_handle_close,
};


static void
dmabuf_format(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf, uint32_t format)
{
    struct display *d = data;

    if (format == DRM_FORMAT_XRGB8888)
        d->xrgb8888_format_found = 1;
}

static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener =
{
    dmabuf_format
};

static void xdg_shell_ping(void *data, struct zxdg_shell_v6 *shell, uint32_t serial)
{
    zxdg_shell_v6_pong(shell, serial);
}

static const struct zxdg_shell_v6_listener xdg_shell_listener =
{
    xdg_shell_ping,
};

static void handle_ivi_surface_configure(void *data, struct ivi_surface *ivi_surface,
			     int32_t width, int32_t height)
{
	/* Simple-shm is resizable */
}

static const struct ivi_surface_listener ivi_surface_listener = {
	handle_ivi_surface_configure,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
                       uint32_t id, const char *interface, uint32_t version)
{
    struct display *d = data;

    if (strcmp(interface, "wl_compositor") == 0)
    {
        d->compositor =
            wl_registry_bind(registry,
                             id, &wl_compositor_interface, 1);
    }
    else if (strcmp(interface, "zxdg_shell_v6") == 0)
    {
        d->shell = wl_registry_bind(registry,
                                    id, &zxdg_shell_v6_interface, 1);
        zxdg_shell_v6_add_listener(d->shell, &xdg_shell_listener, d);
    }
    else if (strcmp(interface, "zwp_fullscreen_shell_v1") == 0)
    {
        d->fshell = wl_registry_bind(registry,
                                     id, &zwp_fullscreen_shell_v1_interface, 1);
    }
    else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0)
    {
        logd("==== d->dmabuf");
        d->dmabuf = wl_registry_bind(registry,
                                     id, &zwp_linux_dmabuf_v1_interface, 1);
        zwp_linux_dmabuf_v1_add_listener(d->dmabuf, &dmabuf_listener, d);
    }
    /*
	else if (strcmp(interface, "wp_viewporter") == 0)
    {
        d->viewporter = wl_registry_bind(registry,
                                     id, &wp_viewporter_interface, 1);
		if (NULL == d->viewporter)
			loge("registry_handle_global viewport is NULL!");
    }
	else if(strcmp(interface, "wl_seat") == 0)
	{
	    d->seat = wl_registry_bind(registry, id, &wl_seat_interface,1);
	}*/
	else if (strcmp(interface, "ivi_application") == 0)
	{
	    logd("=== ivi_application");
		d->ivi_application =
			wl_registry_bind(registry, id,
					 &ivi_application_interface, 1);
	}

}

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
        uint32_t name)
{
}

static const struct wl_registry_listener registry_listener =
{
    registry_handle_global,
    registry_handle_global_remove
};

static struct display* create_display()
{
    struct display* display;
    display = (struct display*)malloc(sizeof(struct display));
    if(display == NULL)
    {
        loge("oom");
        return NULL;
    }
    memset(display, 0, sizeof(struct display));

    display->display = wl_display_connect(NULL);
    assert(display->display);

    /* XXX: fake, because the compositor does not yet advertise anything , ??? */
    display->xrgb8888_format_found = 1;

    display->registry = wl_display_get_registry(display->display);
    wl_registry_add_listener(display->registry,
                             &registry_listener, display);
    wl_display_roundtrip(display->display);
    if (display->dmabuf == NULL)
    {
        loge("No zwp_linux_dmabuf global");
        return NULL;
    }

    // we need two roundtrip here. if you want to kown why,
    // please see the demo simple_shm.c
    wl_display_roundtrip(display->display);

    if (!display->xrgb8888_format_found)
    {
        loge("DRM_FORMAT_XRGB8888 not available");
        return NULL;
    }

    return display;
}

static struct window *create_window(struct display *display)
{
    struct window *window;

    window = zalloc(sizeof *window);
    if (!window)
        return NULL;

    window->callback = NULL;
    window->display = display;
    window->pixel_format = PIXEL_FORMAT_YV12;

    window->pool = AwPoolCreate(NULL);
	window->display_queue = CdxQueueCreate(window->pool);
	window->return_queue = CdxQueueCreate(window->pool);

    window->surface = wl_compositor_create_surface(display->compositor);

    logd("create_window window %p!",window);

	if (display->ivi_application)
	{
		if (window->ivi_surface == NULL)
		{
			cdx_uint32 id_ivisurf = IVI_SURFACE_ID + (cdx_uint32)getpid();
			window->ivi_surface =
				ivi_application_surface_create(display->ivi_application,
								   id_ivisurf, window->surface);
		}
		if (window->ivi_surface == NULL)
		{
			loge("Failed to create ivi_client_surface");
			abort();
		}

		ivi_surface_add_listener(window->ivi_surface,
					 &ivi_surface_listener, window);
	}
    else if (display->shell)
    {
        window->xdg_surface =
            zxdg_shell_v6_get_xdg_surface(display->shell,
                                          window->surface);
        assert(window->xdg_surface);

        zxdg_surface_v6_add_listener(window->xdg_surface,
                                     &xdg_surface_listener, window);
        window->xdg_toplevel =
            zxdg_surface_v6_get_toplevel(window->xdg_surface);
        assert(window->xdg_toplevel);

        zxdg_toplevel_v6_add_listener(window->xdg_toplevel,
                                      &xdg_toplevel_listener, window);

        zxdg_toplevel_v6_set_title(window->xdg_toplevel, "simple-dmabuf");

        window->wait_for_configure = true;
        wl_surface_commit(window->surface);


    }
    else if (display->fshell)
    {
        zwp_fullscreen_shell_v1_present_surface(display->fshell,
                                                window->surface,
                                                ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT,
                                                NULL);
    }
    else
    {
        assert(0);
    }
    /*
	if (display->viewporter == NULL) {
		loge("Compositor does not support wp_viewport");
		return NULL;
	}
	else
	{
		window->viewport = wp_viewporter_get_viewport(display->viewporter,
							      window->surface);
	}*/

    return window;
}

LayerCtrl* LayerCreate_Weston(int *quit)
{
    LayerContext* lc;

    lc = (LayerContext*)malloc(sizeof(LayerContext));
    if(lc==NULL)
    {
        loge("malloc failed");
        return NULL;
    }
    memset(lc, 0, sizeof(LayerContext));

    lc->base.ops = &mLayerControlOps;

    lc->pbQuit = quit;

    lc->display = create_display();
    if(lc->display == NULL)
    {
        loge("display create fail");
        return NULL;
    }

    lc->window = create_window(lc->display);
    if(lc->window == NULL)
    {
        logd("=== create window fail");
        return NULL;
    }
    lc->window->pbQuit = quit;

    CdxIonOpen();
    lc->nUnFreeBufferNum = 0;

    return &lc->base;
}

