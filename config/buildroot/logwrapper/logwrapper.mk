################################################################################
#
# logwrapper package
#
################################################################################
LOGWRAPPER_SITE_METHOD = local
LOGWRAPPER_SITE = $(PLATFORM_PATH)/../../core/system/logwrapper

LOGWRAPPER_LICENSE = GPLv2+, GPLv3+
LOGWRAPPER_LICENSE_FILES = Copyright COPYING

LOGWRAPPER_CFLAGS = $(TARGET_CFLAGS)
LOGWRAPPER_CFLAGS += -O2 -fPIC -Wall
LOGWRAPPER_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D)
LOGWRAPPER_LDFLAGS = $(TARGET_LDFLAGS)
LOGWRAPPER_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -lrt -ldl

define LOGWRAPPER_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(LOGWRAPPER_CFLAGS)" \
		LDFLAGS="$(LOGWRAPPER_LDFLAGS)" -C $(@D) all
endef

define LOGWRAPPER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/logwrapper $(TARGET_DIR)/usr/bin/logwrapper
endef

$(eval $(generic-package))