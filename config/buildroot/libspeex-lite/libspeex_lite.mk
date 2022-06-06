################################################################################
#
# libspeex-lite package
#
################################################################################
LIBSPEEX_LITE_SITE_METHOD = local
LIBSPEEX_LITE_SITE = $(PLATFORM_PATH)/../../base/libspeex-lite

LIBSPEEX_LITE_LICENSE = GPLv2+, GPLv3+
LIBSPEEX_LITE_LICENSE_FILES = Copyright COPYING
LIBSPEEX_LITE_INSTALL_STAGING = YES

#LIBSPEEX_LITE_DEPENDENCIES = 
LIBSPEEX_LITE_CFLAGS = $(TARGET_CFLAGS)
LIBSPEEX_LITE_CFLAGS += -O2 -fPIC -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable
LIBSPEEX_LITE_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D)
LIBSPEEX_LITE_LDFLAGS = $(TARGET_LDFLAGS)
LIBSPEEX_LITE_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared

define LIBSPEEX_LITE_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LIBSPEEX_LITE_CFLAGS)" \
		LDFLAGS="$(LIBSPEEX_LITE_LDFLAGS)" -C $(@D)/src -f $(@D)/src/Makefile
endef

define LIBSPEEX_LITE_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/libspeex-lite
	cp -rf $(@D)/src/*.h $(STAGING_DIR)/usr/include/libspeex-lite/
endef

define LIBSPEEX_LITE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/src/libspeex-lite.so $(TARGET_DIR)/usr/lib/libspeex-lite.so
endef

$(eval $(generic-package))