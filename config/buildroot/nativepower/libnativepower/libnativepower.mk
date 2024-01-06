################################################################################
#
# libnativepower package
#
################################################################################
LIBNATIVEPOWER_SITE_METHOD = local
LIBNATIVEPOWER_SITE = $(PLATFORM_PATH)/../../core/system/nativepower/libnativepower

LIBNATIVEPOWER_LICENSE = GPLv2+, GPLv3+
LIBNATIVEPOWER_LICENSE_FILES = Copyright COPYING
LIBNATIVEPOWER_INSTALL_STAGING = YES

#LIBNATIVEPOWER_DEPENDENCIES = 
LIBNATIVEPOWER_CFLAGS = $(TARGET_CFLAGS)
LIBNATIVEPOWER_CFLAGS += -O2 -fPIC -Wall -DCOMPLETE_TIMEOUT
LIBNATIVEPOWER_CFLAGS += -I$(STAGING_DIR)/usr/include  -I$(STAGING_DIR)/usr/include/dbus-1.0 -I$(STAGING_DIR)/usr/lib/dbus-1.0/include -I$(@D) -I$(@D)/include
LIBNATIVEPOWER_LDFLAGS = $(TARGET_LDFLAGS)
LIBNATIVEPOWER_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared

define LIBNATIVEPOWER_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LIBNATIVEPOWER_CFLAGS)" \
		LDFLAGS="$(LIBNATIVEPOWER_LDFLAGS)" -C $(@D) all
endef

define LIBNATIVEPOWER_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/libnativepower
	cp -rf $(@D)/include/*.h $(STAGING_DIR)/usr/include/libnativepower
endef

define LIBNATIVEPOWER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/libnativepower.so $(TARGET_DIR)/usr/lib/libnativepower.so
endef

$(eval $(generic-package))