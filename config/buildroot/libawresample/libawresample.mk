################################################################################
#
# libawresample package
#
################################################################################
LIBAWRESAMPLE_SITE_METHOD = local
LIBAWRESAMPLE_SITE = $(PLATFORM_PATH)/../../base/libawresample

LIBAWRESAMPLE_LICENSE = GPLv2+, GPLv3+
LIBAWRESAMPLE_LICENSE_FILES = Copyright COPYING
LIBAWRESAMPLE_INSTALL_STAGING = YES

#LIBAWRESAMPLE_DEPENDENCIES = 
LIBAWRESAMPLE_CFLAGS = $(TARGET_CFLAGS)
LIBAWRESAMPLE_CFLAGS += -O2 -fPIC -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable
LIBAWRESAMPLE_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D)
LIBAWRESAMPLE_LDFLAGS = $(TARGET_LDFLAGS)
LIBAWRESAMPLE_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared

define LIBAWRESAMPLE_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LIBAWRESAMPLE_CFLAGS)" \
		LDFLAGS="$(LIBAWRESAMPLE_LDFLAGS)" -C $(@D)/resample/src -f $(@D)/resample/src/Makefile
endef

define LIBAWRESAMPLE_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/libawresample
	cp -rf $(@D)/resample/src/do_audioresample.h $(STAGING_DIR)/usr/include/libawresample/do_audioresample.h
endef

define LIBAWRESAMPLE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/resample/src/libawresample.so $(TARGET_DIR)/usr/lib/libawresample.so
endef

$(eval $(generic-package))