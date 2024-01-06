################################################################################
#
# libsuspend package
#
################################################################################
LIBSUSPEND_SITE_METHOD = local
LIBSUSPEND_SITE = $(PLATFORM_PATH)/../../core/system/nativepower/libsuspend

LIBSUSPEND_LICENSE = GPLv2+, GPLv3+
LIBSUSPEND_LICENSE_FILES = Copyright COPYING
LIBSUSPEND_INSTALL_STAGING = YES

#LIBSUSPEND_DEPENDENCIES = 
LIBSUSPEND_CFLAGS = $(TARGET_CFLAGS)
LIBSUSPEND_CFLAGS += -O2 -fPIC -Wall -DCOMPLETE_TIMEOUT
LIBSUSPEND_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D) -I$(@D)/include
LIBSUSPEND_LDFLAGS = $(TARGET_LDFLAGS)
LIBSUSPEND_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared

define LIBSUSPEND_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LIBSUSPEND_CFLAGS)" \
		LDFLAGS="$(LIBSUSPEND_LDFLAGS)" -C $(@D) all
endef

define LIBSUSPEND_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/libsuspend
	cp -rf $(@D)/include/* $(STAGING_DIR)/usr/include/libsuspend
endef

define LIBSUSPEND_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/libsuspend.so $(TARGET_DIR)/usr/lib/libsuspend.so
endef

$(eval $(generic-package))