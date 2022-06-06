################################################################################
#
# apps
#
################################################################################
MTOP_SITE_METHOD = local
MTOP_SITE = $(PLATFORM_PATH)/../../apps/mtop
MTOP_LICENSE = GPLv2+, GPLv3+
MTOP_LICENSE_FILES = Copyright COPYING

define MTOP_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" -C $(@D) all
endef

define MTOP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/mtop $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
