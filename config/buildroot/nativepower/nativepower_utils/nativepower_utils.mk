################################################################################
#
# nativepower_utils package
#
################################################################################
NATIVEPOWER_UTILS_SITE_METHOD = local
NATIVEPOWER_UTILS_SITE = $(PLATFORM_PATH)/../../core/system/nativepower/nativepower_utils

NATIVEPOWER_UTILS_LICENSE = GPLv2+, GPLv3+
NATIVEPOWER_UTILS_LICENSE_FILES = Copyright COPYING

NATIVEPOWER_UTILS_DEPENDENCIES = \
	libscenemanager \
    libuci

NATIVEPOWER_UTILS_CFLAGS = $(TARGET_CFLAGS)
NATIVEPOWER_UTILS_CFLAGS += -O2 -fPIC -Wall -DCOMPLETE_TIMEOUT
NATIVEPOWER_UTILS_CFLAGS += -I$(STAGING_DIR)/usr/include  -I$(STAGING_DIR)/usr/include/dbus-1.0 -I$(STAGING_DIR)/usr/lib/dbus-1.0/include -I$(@D) -I$(@D)/include
NATIVEPOWER_UTILS_CFLAGS += -I$(STAGING_DIR)/usr/include/libsuspend -I$(STAGING_DIR)/usr/include/libnativepower -I$(STAGING_DIR)/usr/include/libscenemanager
NATIVEPOWER_UTILS_LDFLAGS = $(TARGET_LDFLAGS)
NATIVEPOWER_UTILS_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -lscenemanager -ldbus-1 -luci

ifeq ($(BR2_NATIVEPOWER_USE_DBUS), y)
NATIVEPOWER_UTILS_DEPENDENCIES += dbus
NATIVEPOWER_UTILS_CFLAGS += -DUSE_DBUS
NATIVEPOWER_UTILS_LDFLAGS += -lnativepower
endif

define NATIVEPOWER_UTILS_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(NATIVEPOWER_UTILS_CFLAGS)" \
		LDFLAGS="$(NATIVEPOWER_UTILS_LDFLAGS)" -C $(@D) all
endef

define NATIVEPOWER_UTILS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/nativepower_utils $(TARGET_DIR)/usr/bin/nativepower_utils
endef

$(eval $(generic-package))