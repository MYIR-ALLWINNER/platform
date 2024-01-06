################################################################################
#
# boot-play package
#
################################################################################
MEDIAPLAYER_SITE_METHOD = local
MEDIAPLAYER_SITE = $(PLATFORM_PATH)/../../apps/mediaplayer
MEDIAPLAYER_LICENSE = GPLv2+, GPLv3+
MEDIAPLAYER_LICENSE_FILES = Copyright COPYING
MEDIAPLAYER_DEPENDENCIES = libcedarc cedarx libdrm weston wayland-ivi-extension
MEDIAPLAYER_CFLAGS = $(TARGET_CFLAGS)
MEDIAPLAYER_CFLAGS += -O2 -fPIC -Wall -DCOMPLETE_TIMEOUT
MEDIAPLAYER_CFLAGS += -I$(STAGING_DIR)/usr/include/cedarx -I$(@D)
MEDIAPLAYER_CFLAGS += -I$(STAGING_DIR)/usr/include/libcedarc
MEDIAPLAYER_CFLAGS += -I$(STAGING_DIR)/usr/include/libdrm
MEDIAPLAYER_CFLAGS += -I$(STAGING_DIR)/usr/include/weston
MEDIAPLAYER_CFLAGS += -I$(PLATFORM_PATH)/framework/weston
MEDIAPLAYER_CFLAGS += -I$(TARGET_DIR)/usr/include
MEDIAPLAYER_LDFLAGS = $(TARGET_LDFLAGS)
MEDIAPLAYER_LDFLAGS += -lweston-5 -ldrm -lwayland-server -lpixman-1 \
			-lgbm -lxkbcommon -lpcre -linput -ludev \
			-levdev -lmtdev -lffi -lwayland-egl -lwayland-client \
			-lweston-desktop-5 -lrt -lpthread -lz -lm -lvdecoder \
			-ladecoder -lm -lsubdecoder -lVE -lcrypto \
			-lvideoengine -lMemAdapter -lcdc_base -lssl  \
			-lsalsa  -lcdx_base -livi-application -lcdx_stream \
			-lcdx_playback -lxplayer -lcdx_common -lcdx_parser \
			-l:fullscreen-shell.so

MEDIAPLAYER_LDFLAGS += -L$(TARGET_DIR)/usr/lib/weston
MEDIAPLAYER_LDFLAGS += -L$(TARGET_DIR)/usr/lib

define MEDIAPLAYER_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(MEDIAPLAYER_CFLAGS)" \
		LDFLAGS="$(MEDIAPLAYER_LDFLAGS)" -C $(@D) all
endef

define MEDIAPLAYER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/mediaplayer $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
