################################################################################
#
# nativepower_daemon package
#
################################################################################
NATIVEPOWER_DAEMON_SITE_METHOD = local
NATIVEPOWER_DAEMON_SITE = $(PLATFORM_PATH)/../../core/system/nativepower/nativepower_daemon

NATIVEPOWER_DAEMON_LICENSE = GPLv2+, GPLv3+
NATIVEPOWER_DAEMON_LICENSE_FILES = Copyright COPYING

NATIVEPOWER_DAEMON_DEPENDENCIES = \
	libsuspend \
    libscenemanager \
    libuci

NATIVEPOWER_DAEMON_CFLAGS = $(TARGET_CFLAGS)
NATIVEPOWER_DAEMON_CFLAGS += -O2 -fPIC -Wall -DCOMPLETE_TIMEOUT
NATIVEPOWER_DAEMON_CFLAGS += -I$(STAGING_DIR)/usr/include  -I$(STAGING_DIR)/usr/include/dbus-1.0 -I$(STAGING_DIR)/usr/lib/dbus-1.0/include -I$(@D) -I$(@D)/include
NATIVEPOWER_DAEMON_CFLAGS += -I$(STAGING_DIR)/usr/include/libsuspend -I$(STAGING_DIR)/usr/include/libnativepower -I$(STAGING_DIR)/usr/include/libscenemanager
NATIVEPOWER_DAEMON_LDFLAGS = $(TARGET_LDFLAGS)
NATIVEPOWER_DAEMON_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -lsuspend -lscenemanager -ldbus-1 -luci

ifeq ($(BR2_NATIVEPOWER_USE_DBUS), y)
NATIVEPOWER_DAEMON_DEPENDENCIES += dbus
NATIVEPOWER_DAEMON_LDFLAGS	+= -ldbus-1
NATIVEPOWER_DAEMON_CFLAGS	+= -DUSE_DBUS
endif

define NATIVEPOWER_DAEMON_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(NATIVEPOWER_DAEMON_CFLAGS)" \
		LDFLAGS="$(NATIVEPOWER_DAEMON_LDFLAGS)" -C $(@D) all
endef

define NATIVEPOWER_DAEMON_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/nativepower_daemon $(TARGET_DIR)/usr/bin/nativepower_daemon
endef

$(eval $(generic-package))