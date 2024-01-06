#ifndef VIVANTE_CLIENT_PROTOCOL_H
#define VIVANTE_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_viv;

extern const struct wl_interface wl_viv_interface;

#define WL_VIV_CREATE_BUFFER	0

static inline void
wl_viv_set_user_data(struct wl_viv *wl_viv, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_viv, user_data);
}

static inline void *
wl_viv_get_user_data(struct wl_viv *wl_viv)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_viv);
}

static inline void
wl_viv_destroy(struct wl_viv *wl_viv)
{
	wl_proxy_destroy((struct wl_proxy *) wl_viv);
}

static inline struct wl_buffer *
wl_viv_create_buffer(struct wl_viv *wl_viv, uint32_t width, uint32_t height, uint32_t stride, int32_t format, int32_t node, int32_t pool, uint32_t bytes)
{
	struct wl_proxy *id;

	id = wl_proxy_create((struct wl_proxy *) wl_viv,
			     &wl_buffer_interface);
	if (!id)
		return NULL;

	wl_proxy_marshal((struct wl_proxy *) wl_viv,
			 WL_VIV_CREATE_BUFFER, id, width, height, stride, format, node, pool, bytes);

	return (struct wl_buffer *) id;
}

#ifdef  __cplusplus
}
#endif

#endif
