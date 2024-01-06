#ifndef WESTON_DISPLAY_H
#define WESTON_DISPLAY_H

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include "cdx_config.h"
#include <cdx_log.h>
#include "layerControl.h"

#include "memoryAdapter.h"
//#include "display_H3.h"

#include <xf86drm.h>
#include <i915_drm.h>
#include <drm_fourcc.h>
#include <wayland-client.h>
#include "shared/zalloc.h"
#include "xdg-shell-unstable-v6-client-protocol.h"
#include "fullscreen-shell-unstable-v1-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
/*
struct display
{
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct zxdg_shell_v6 *shell;
    struct zwp_fullscreen_shell_v1 *fshell;
    struct zwp_linux_dmabuf_v1 *dmabuf;
    int xrgb8888_format_found;
};

typedef struct VPictureNode_t VPictureNode;

struct VPictureNode_t
{
    VideoPicture* pPicture;
    VPictureNode* pNext;
};
*/


#endif
