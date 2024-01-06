################################################################################
#
# misc-upgrade package
#
################################################################################
MISC_UPGRADE_SITE_METHOD = local
MISC_UPGRADE_SITE = $(PLATFORM_PATH)/../../core/system/ota/misc-upgrade

MISC_UPGRADE_LICENSE = GPLv2+, GPLv3+
MISC_UPGRADE_LICENSE_FILES = Copyright COPYING
MISC_UPGRADE_INSTALL_STAGING = YES

#MISC_UPGRADE_DEPENDENCIES = 
MISC_UPGRADE_CFLAGS = $(TARGET_CFLAGS)
MISC_UPGRADE_CFLAGS += -O2 -fPIC -Wall
MISC_UPGRADE_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D)
MISC_UPGRADE_LDFLAGS = $(TARGET_LDFLAGS)
MISC_UPGRADE_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -shared

#define MISC_UPGRADE_BUILD_CMDS
#	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(MISC_UPGRADE_CFLAGS)" \
#		LDFLAGS="$(MISC_UPGRADE_LDFLAGS)" -C $(@D) all
#endef

define MISC_UPGRADE_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) SRCDIR=$(@D) \
		-C $(@D)/tools -f $(@D)/tools/Makefile;
endef

define MISC_UPGRADE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/tools/write_misc $(TARGET_DIR)/usr/bin/write_misc
	$(INSTALL) -D -m 0755 $(@D)/tools/read_misc $(TARGET_DIR)/usr/bin/read_misc
endef

$(eval $(generic-package))