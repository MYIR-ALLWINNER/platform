################################################################################
#
# mtp package
#
################################################################################
MTP_SITE_METHOD = local
MTP_SITE = $(PLATFORM_PATH)/../../core/system/mtp

MTP_LICENSE = GPLv2+, GPLv3+
MTP_LICENSE_FILES = Copyright COPYING

MTP_DEPENDENCIES = \
	libcutils \

MTP_CFLAGS = $(TARGET_CFLAGS)
MTP_CFLAGS += -O2 -fPIC -Wall -DMTP_SERVER_DAEMON -DFORCE_DEBUG -DNO_DEBUG=1
MTP_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(@D) -I$(@D)/Disk
MTP_LDFLAGS = $(TARGET_LDFLAGS)
MTP_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -lcutils

define MTP_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(MTP_CFLAGS)" \
		LDFLAGS="$(MTP_LDFLAGS)" -C $(@D) all
endef

define MTP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/mtp $(TARGET_DIR)/usr/bin/mtp
endef

$(eval $(generic-package))