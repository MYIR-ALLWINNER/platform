################################################################################
#
# boot-play package
#
################################################################################
BOOT_PLAY_SITE_METHOD = local
BOOT_PLAY_SITE = $(PLATFORM_PATH)/../../apps/boot-play
BOOT_PLAY_LICENSE = GPLv2+, GPLv3+
BOOT_PLAY_LICENSE_FILES = Copyright COPYING
BOOT_PLAY_DEPENDENCIES = libubox libpng
BOOT_PLAY_CFLAGS = $(TARGET_CFLAGS)
BOOT_PLAY_CFLAGS += -O2 -fPIC -Wall -DCOMPLETE_TIMEOUT
BOOT_PLAY_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D)
BOOT_PLAY_LDFLAGS = $(TARGET_LDFLAGS)
BOOT_PLAY_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpng -lubox

define BOOT_PLAY_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(BOOT_PLAY_CFLAGS)" \
		LDFLAGS="$(BOOT_PLAY_LDFLAGS)" -C $(@D) all
endef

define BOOT_PLAY_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/initplay $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
