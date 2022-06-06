################################################################################
#
# libscenemanager package
#
################################################################################
LIBSCENEMANAGER_SITE_METHOD = local
LIBSCENEMANAGER_SITE = $(PLATFORM_PATH)/../../core/system/nativepower/libscenemanager

LIBSCENEMANAGER_LICENSE = GPLv2+, GPLv3+
LIBSCENEMANAGER_LICENSE_FILES = Copyright COPYING
LIBSCENEMANAGER_INSTALL_STAGING = YES

LIBSCENEMANAGER_DEPENDENCIES = \
    libuci

LIBSCENEMANAGER_CFLAGS = $(TARGET_CFLAGS)
LIBSCENEMANAGER_CFLAGS += -O2 -fPIC -Wall -DCOMPLETE_TIMEOUT
LIBSCENEMANAGER_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D) -I$(@D)/include
LIBSCENEMANAGER_CFLAGS += -DSUN8IW11P1
LIBSCENEMANAGER_LDFLAGS = $(TARGET_LDFLAGS)
LIBSCENEMANAGER_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared -luci

define LIBSCENEMANAGER_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LIBSCENEMANAGER_CFLAGS)" \
		LDFLAGS="$(LIBSCENEMANAGER_LDFLAGS)" -C $(@D) all
endef

define LIBSCENEMANAGER_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/libscenemanager
	cp -rf $(@D)/include/*.h $(STAGING_DIR)/usr/include/libscenemanager
endef

define LIBSCENEMANAGER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/libscenemanager.so $(TARGET_DIR)/usr/lib/libscenemanager.so
endef

$(eval $(generic-package))