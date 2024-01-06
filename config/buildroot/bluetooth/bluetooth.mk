################################################################################
#
# bluetooth package
#
################################################################################
BLUETOOTH_SITE_METHOD = local
BLUETOOTH_SITE = $(PLATFORM_PATH)/../../core/net/bluetooth

BLUETOOTH_LICENSE = GPLv2+, GPLv3+
BLUETOOTH_LICENSE_FILES = Copyright COPYING
BLUETOOTH_INSTALL_STAGING = YES

#ifeq ($(BR2_PACKAGE_BLUETOOTH_REALTEK_RTL8723DS),y)
#BLUETOOTH_TARGETS += realtek
#BLUETOOTH_DEPENDENCIES += bluez_utils
#endif

#ifeq ($(BR2_PACKAGE_BLUETOOTH_AMPAK),y)
BLUETOOTH_CFLAGS += -O2 -I$(STAGING_DIR)/usr/include -I$(@D)
BLUETOOTH_DEPENDENCIES += bluez5_utils alsa-lib libuci
BLUETOOTH_LDFLAGS = $(TARGET_LDFLAGS)
BLUETOOTH_LDFLAGS += -L$(TARGET_DIR)/usr/lib/
#endif

define BLUETOOTH_BUILD_CMDS
	if test "$(BR2_PACKAGE_BLUETOOTH_REALTEK_RTL8723DS)" = "y" ; then \
		mkdir -p $(@D)/build-rtl8723ds; \
		$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) SRCDIR=$(@D) \
		-C $(@D)/build-rtl8723ds -f $(@D)/realtek/rtl8723ds/rtk_hciattach/Makefile; \
	fi;

	if test "$(BR2_PACKAGE_BLUETOOTH_AMPAK)" = "y" ; then \
		mkdir -p $(@D)/build-ampak/build; \
		$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) SRCDIR=$(@D) \
		LDFLAGS="$(BLUETOOTH_LDFLAGS)" CFLAGS="$(BLUETOOTH_CFLAGS)" \
		-C $(@D)/build-ampak/build -f $(@D)/ampak/3rdparty/embedded/bsa_examples/linux/libbtapp/build/Makefile; \
		$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) SRCDIR=$(@D) \
		LDFLAGS="$(BLUETOOTH_LDFLAGS)" CFLAGS="$(BLUETOOTH_CFLAGS)" \
		-C $(@D)/ampak/3rdparty/embedded/bsa_examples/linux/libbtapp/bt_test -f $(@D)/ampak/3rdparty/embedded/bsa_examples/linux/libbtapp/bt_test/Makefile; \
	fi;
endef

define BLUETOOTH_INSTALL_TARGET_CMDS
	if test "$(BR2_PACKAGE_BLUETOOTH_REALTEK_RTL8723DS)" = "y" ; then \
		mkdir -p $(TARGET_DIR)/lib/firmware/rtlbt; \
		cp -rf $(@D)/realtek/rtl8723ds/firmware/* $(TARGET_DIR)/lib/firmware/rtlbt; \
		$(INSTALL) -D -m 0755 $(@D)/build-rtl8723ds/rtk_hciattach $(TARGET_DIR)/usr/bin/rtk_hciattach; \
	fi;

	if test "$(BR2_PACKAGE_BLUETOOTH_AMPAK)" = "y" ; then \
		mkdir -p $(TARGET_DIR)/lib/firmware; \
		mkdir -p $(TARGET_DIR)/etc/bluetooth; \
		cp -rf $(@D)/ampak/firmware/ap6236/fw_bcm43436b0.bin $(TARGET_DIR)/lib/firmware/fw_bcm43436b0.bin; \
		cp -rf $(@D)/ampak/firmware/ap6236/fw_bcm43436b0_apsta.bin $(TARGET_DIR)/lib/firmware/fw_bcm43436b0_apsta.bin; \
		cp -rf $(@D)/ampak/firmware/ap6236/nvram_ap6236.txt $(TARGET_DIR)/lib/firmware/nvram.txt; \
		cp -rf $(@D)/ampak/firmware/ap6236/BCM43430B0.hcd $(TARGET_DIR)/lib/firmware/BCM43430B0.hcd; \
		cp -rf $(@D)/ampak/btenable.sh $(TARGET_DIR)/etc/bluetooth/btenable.sh; \
		cp -rf $(@D)/ampak/3rdparty/embedded/bsa_examples/linux/libbtapp/bt_test/bt_test $(TARGET_DIR)/usr/bin/bt_test; \
	fi;
endef

$(eval $(generic-package))
