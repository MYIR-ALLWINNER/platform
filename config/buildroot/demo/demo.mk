################################################################################
#
# apps
#
################################################################################
DEMO_SITE_METHOD = local
DEMO_SITE = $(PLATFORM_PATH)/../../apps/demo
DEMO_LICENSE = GPLv2+, GPLv3+
DEMO_LICENSE_FILES = Copyright COPYING

define DEMO_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" -C $(@D) all
endef

define DEMO_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/demo $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
