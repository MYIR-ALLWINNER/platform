################################################################################
#
# libcutils package
#
################################################################################
LIBCUTILS_SITE_METHOD = local
LIBCUTILS_SITE = $(PLATFORM_PATH)/../../external/libcutils

LIBCUTILS_LICENSE = GPLv2+, GPLv3+
LIBCUTILS_LICENSE_FILES = Copyright COPYING
LIBCUTILS_INSTALL_STAGING = YES

#LIBCUTILS_DEPENDENCIES = 
LIBCUTILS_CFLAGS = $(TARGET_CFLAGS)
LIBCUTILS_CFLAGS += -O2 -fPIC -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable
LIBCUTILS_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D) -I$(@D)/include/cutils -I$(@D)/include -D_GNU_SOURCE
LIBCUTILS_LDFLAGS = $(TARGET_LDFLAGS)
LIBCUTILS_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared

define LIBCUTILS_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LIBCUTILS_CFLAGS)" \
		LDFLAGS="$(LIBCUTILS_LDFLAGS)" -C $(@D) all
endef

define LIBCUTILS_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/cutils
	cp -rf $(@D)/include/cutils/* $(STAGING_DIR)/usr/include/cutils/
endef

define LIBCUTILS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/libcutils.so $(TARGET_DIR)/usr/lib/libcutils.so
endef

$(eval $(generic-package))