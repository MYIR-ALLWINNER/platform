################################################################################
#
# ota-burnboot package
#
################################################################################
OTA_BURNBOOT_SITE_METHOD = local
OTA_BURNBOOT_SITE = $(PLATFORM_PATH)/../../core/system/ota/ota-burnboot

OTA_BURNBOOT_LICENSE = GPLv2+, GPLv3+
OTA_BURNBOOT_LICENSE_FILES = Copyright COPYING
OTA_BURNBOOT_INSTALL_STAGING = YES

#OTA_BURNBOOT_DEPENDENCIES = 
OTA_BURNBOOT_CFLAGS = $(TARGET_CFLAGS)
OTA_BURNBOOT_CFLAGS += -O2 -fPIC -Wall
OTA_BURNBOOT_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D) -I$(@D)/include -I$(STAGING_DIR)/usr/include/ota-burnboot
OTA_BURNBOOT_LDFLAGS = $(TARGET_LDFLAGS)
OTA_BURNBOOT_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared

define OTA_BURNBOOT_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/ota-burnboot
	cp -rf $(@D)/src/include/*.h $(STAGING_DIR)/usr/include/ota-burnboot
endef

define OTA_BURNBOOT_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) SRCDIR=$(@D) \
		-C $(@D)/src -f $(@D)/src/Makefile;
endef

define OTA_BURNBOOT_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/src/libota-burnboot.so $(TARGET_DIR)/usr/lib/libota-burnboot.so
	$(INSTALL) -D -m 0755 $(@D)/src/ota-burnboot0 $(TARGET_DIR)/usr/bin/ota-burnboot0
	$(INSTALL) -D -m 0755 $(@D)/src/ota-burnuboot $(TARGET_DIR)/usr/bin/ota-burnuboot
endef

$(eval $(generic-package))