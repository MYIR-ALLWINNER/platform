################################################################################
#
# resample package
#
################################################################################
RESAMPLE_VERSION:=0.0.1
RESAMPLE_SITE_METHOD = local
RESAMPLE_SITE = $(PLATFORM_PATH)/../../base/resample

RESAMPLE_LICENSE = GPLv2+, GPLv3+
RESAMPLE_LICENSE_FILES = Copyright COPYING
RESAMPLE_INSTALL_STAGING = YES

#RESAMPLE_DEPENDENCIES = 
RESAMPLE_CFLAGS = $(TARGET_CFLAGS)
RESAMPLE_CFLAGS += -O2 -fPIC -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable
RESAMPLE_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D)
RESAMPLE_LDFLAGS = $(TARGET_LDFLAGS)
RESAMPLE_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared

define RESAMPLE_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(RESAMPLE_CFLAGS)" \
		LDFLAGS="$(RESAMPLE_LDFLAGS)" -C $(@D)/AC320 -f $(@D)/AC320/Makefile
endef

define RESAMPLE_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include
	cp -rf $(@D)/AC320/*.h $(STAGING_DIR)/usr/include/
endef

define RESAMPLE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/AC320/.lib/*.so $(TARGET_DIR)/usr/lib/
endef

$(eval $(generic-package))