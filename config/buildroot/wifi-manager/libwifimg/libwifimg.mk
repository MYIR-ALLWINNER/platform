################################################################################
#
# libwifimg package
#
################################################################################
LIBWIFIMG_SITE_METHOD = local
LIBWIFIMG_SITE = $(PLATFORM_PATH)/../../core/net/wifi-manager/src

LIBWIFIMG_LICENSE = GPLv2+, GPLv3+
LIBWIFIMG_LICENSE_FILES = Copyright COPYING
LIBWIFIMG_INSTALL_STAGING = YES

LIBWIFIMG_CFLAGS = $(TARGET_CFLAGS)
LIBWIFIMG_CFLAGS += -O2 -fPIC -Wall -DHAVE_SYS_UIO_H -DHAVE_IOCTL -DANDROID_SMP=1
LIBWIFIMG_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D) -I$(@D)/include
LIBWIFIMG_LDFLAGS = $(TARGET_LDFLAGS)
LIBWIFIMG_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -shared -lpthread

define LIBWIFIMG_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LIBWIFIMG_CFLAGS)" \
		LDFLAGS="$(LIBWIFIMG_LDFLAGS)" -C $(@D) all
endef

define LIBWIFIMG_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/wifi-manager
	cp -rf $(@D)/include/*.h $(STAGING_DIR)/usr/include/wifi-manager/
endef

define LIBWIFIMG_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/libwifimg.so $(TARGET_DIR)/usr/lib/libwifimg.so
endef

$(eval $(generic-package))