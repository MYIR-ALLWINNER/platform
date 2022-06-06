################################################################################
#
# smartlinkd package
#
################################################################################
SMARTLINKD_SITE_METHOD = local
SMARTLINKD_SITE = $(PLATFORM_PATH)/../../core/net/smartlinkd

SMARTLINKD_LICENSE = GPLv2+, GPLv3+
SMARTLINKD_LICENSE_FILES = Copyright COPYING
#SMARTLINKD_INSTALL_STAGING = YES

ifeq ($(BR2_PACKAGE_LIBSMARTLINKD_CLIENT),y)
SMARTLINKD_TARGETS += libclient
#SMARTLINKD_DEPENDENCIES += 
endif

ifeq ($(BR2_PACKAGE_SMARTLINKD_SERVER),y)
SMARTLINKD_TARGETS += server
#SMARTLINKD_DEPENDENCIES += libclient
endif

ifeq ($(BR2_PACKAGE_SMARTLINKD_SOUNDWAVE),y)
SMARTLINKD_TARGETS += soundwave
SMARTLINKD_DEPENDENCIES += libuci alsa-lib openssl
endif

ifeq ($(BR2_PACKAGE_SMARTLINKD_XR819_SMARTCONFIG),y)
SMARTLINKD_TARGETS += xr819
#SMARTLINKD_DEPENDENCIES += libclient
endif

# Build each package in its own directory not to share object files
define SMARTLINKD_BUILD_CMDS
	$(foreach t,$(SMARTLINKD_TARGETS),\
		mkdir -p $(@D)/build-$(t) && \
		$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) SRCDIR=$(@D) \
			-C $(@D)/build-$(t) -f $(@D)/makefiles/$(t).mk$(sep))
endef

define SMARTLINKD_INSTALL_TARGET_CMDS
	if test "$(BR2_PACKAGE_LIBSMARTLINKD_CLIENT)" = "y" ; then \
		$(INSTALL) -D -m 0644 $(@D)/build-libclient/libsmartlinkd_client.so $(TARGET_DIR)/usr/lib/libsmartlinkd_client.so ; \
	fi;
	
	if test "$(BR2_PACKAGE_SMARTLINKD_SERVER)" = "y" ; then \
		$(INSTALL) -D -m 0755 $(@D)/build-server/smartlinkd $(TARGET_DIR)/usr/bin/smartlinkd ; \
	fi;

	if test "$(BR2_PACKAGE_SMARTLINKD_SOUNDWAVE)" = "y" ; then \
		$(INSTALL) -D -m 0755 $(@D)/build-soundwave/smartlinkd_soundwave $(TARGET_DIR)/usr/bin/smartlinkd_soundwave ; \
	fi;
	
	if test "$(BR2_PACKAGE_SMARTLINKD_XR819_SMARTCONFIG)" = "y" ; then \
		$(INSTALL) -D -m 0755 $(@D)/build-xr819/smartlinkd_xrsc $(TARGET_DIR)/usr/bin/smartlinkd_xrsc ; \
	fi;

endef


$(eval $(generic-package))