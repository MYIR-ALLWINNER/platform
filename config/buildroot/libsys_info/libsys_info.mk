################################################################################
#
# libsys_info package
#
################################################################################
LIBSYS_INFO_SITE_METHOD = local
LIBSYS_INFO_SITE = $(PLATFORM_PATH)/../../base/libsys_info

LIBSYS_INFO_LICENSE = GPLv2+, GPLv3+
LIBSYS_INFO_LICENSE_FILES = Copyright COPYING
LIBSYS_INFO_INSTALL_STAGING = YES

#LIBSYS_INFO_DEPENDENCIES = 
LIBSYS_INFO_CFLAGS = $(TARGET_CFLAGS)
LIBSYS_INFO_CFLAGS += -O2 -fPIC -Wall -DCOMPLETE_TIMEOUT
LIBSYS_INFO_CFLAGS += -I$(STAGING_DIR)/usr/include  -I$(STAGING_DIR)/usr/include/dbus-1.0 -I$(STAGING_DIR)/usr/lib/dbus-1.0/include -I$(@D) -I$(@D)/include
LIBSYS_INFO_LDFLAGS = $(TARGET_LDFLAGS)
LIBSYS_INFO_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared

define LIBSYS_INFO_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LIBSYS_INFO_CFLAGS)" \
		LDFLAGS="$(LIBSYS_INFO_LDFLAGS)" -C $(@D) all
endef

define LIBSYS_INFO_INSTALL_STAGING_CMDS
	cp -rf $(@D)/sys_info.h $(STAGING_DIR)/usr/include/
endef

define LIBSYS_INFO_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/libsys_info.so $(TARGET_DIR)/usr/lib/libsys_info.so
endef

$(eval $(generic-package))