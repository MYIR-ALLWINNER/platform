#ifndef VIVANTE_SERVER_PROTOCOL_H
#define VIVANTE_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

struct wl_client;
struct wl_resource;

struct wl_viv;

extern const struct wl_interface wl_viv_interface;

struct wl_viv_interface {
	/**
	 * create_buffer - (none)
	 * @id: (none)
	 * @width: (none)
	 * @height: (none)
	 * @stride: (none)
	 * @format: (none)
	 * @node: (none)
	 * @pool: (none)
	 * @bytes: (none)
	 */
	void (*create_buffer)(struct wl_client *client,
			      struct wl_resource *resource,
			      uint32_t id,
			      uint32_t width,
			      uint32_t height,
			      uint32_t stride,
			      int32_t format,
			      int32_t node,
			      int32_t pool,
			      uint32_t bytes);
};

#ifdef  __cplusplus
}
#endif

#endif
