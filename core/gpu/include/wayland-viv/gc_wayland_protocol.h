#ifndef __WAYLANDEGLVIV_H__
#define __WAYLANDEGLVIV_H__

#include "gc_hal_eglplatform.h"

#define container_of(ptr, type, member) ({				\
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

gcsWL_EGL_DISPLAY*
gcoWL_GetDisplay(struct wl_display*);

void
gcoWL_CreateGhostBuffer(gcsWL_EGL_DISPLAY*, gcsWL_EGL_BUFFER *);

void
gcoWL_ReleaseDisplay(gcsWL_EGL_DISPLAY*);

#endif /* __WAYLANDEGLVIV_H__ */
