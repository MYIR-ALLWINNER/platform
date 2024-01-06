################################################################################
#
# ota-upgrade package
#
################################################################################
OTA_UPGRADE_SITE_METHOD = local
OTA_UPGRADE_SITE = $(PLATFORM_PATH)/../../core/system/ota/ota-upgrade

OTA_UPGRADE_LICENSE = GPLv2+, GPLv3+
OTA_UPGRADE_LICENSE_FILES = Copyright COPYING

#OTA_UPGRADE_DEPENDENCIES = 
OTA_UPGRADE_CFLAGS = $(TARGET_CFLAGS)
OTA_UPGRADE_CFLAGS += -O2 -fPIC -Wall
OTA_UPGRADE_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D)
OTA_UPGRADE_LDFLAGS = $(TARGET_LDFLAGS)

define OTA_UPGRADE_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(OTA_UPGRADE_CFLAGS)" \
		LDFLAGS="$(OTA_UPGRADE_LDFLAGS)" -C $(@D) all
endef

define OTA_UPGRADE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/rootfs_update $(TARGET_DIR)/usr/bin/rootfs_update
	$(INSTALL) -D -m 0755 $(@D)/upgrade $(TARGET_DIR)/usr/bin/upgrade
	$(INSTALL) -D -m 0755 $(@D)/config.ini $(TARGET_DIR)/etc/config.ini
endef

$(eval $(generic-package))