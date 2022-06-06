################################################################################
#
# longan_info
#
################################################################################
LONGAN_INFO_SITE_METHOD = local
LONGAN_INFO_SITE = $(PLATFORM_PATH)/../../apps/longan_info
LONGAN_INFO_LICENSE = GPLv2+, GPLv3+
LONGAN_INFO_LICENSE_FILES = Copyright COPYING
LONGAN_INFO_DEPENDENCIES = libsys_info

LONGAN_INFO_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lsys_info

define LONGAN_INFO_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LONGAN_INFO_CFLAGS)" \
		LDFLAGS="$(LONGAN_INFO_LDFLAGS)" -C $(@D) all
endef

define LONGAN_INFO_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/longan_info $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
