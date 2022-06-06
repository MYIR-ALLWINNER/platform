################################################################################
#
# wifi-manager-demo package
#
################################################################################
WIFI_MANAGER_DEMO_SITE_METHOD = local
WIFI_MANAGER_DEMO_SITE = $(PLATFORM_PATH)/../../core/net/wifi-manager/src_demo

WIFI_MANAGER_DEMO_LICENSE = GPLv2+, GPLv3+
WIFI_MANAGER_DEMO_LICENSE_FILES = Copyright COPYING

WIFI_MANAGER_DEMO_DEPENDENCIES = \
	libwifimg \
	wpa_supplicant

WIFI_MANAGER_DEMO_CFLAGS = $(TARGET_CFLAGS)
WIFI_MANAGER_DEMO_CFLAGS += -O2 -fPIC -Wall
WIFI_MANAGER_DEMO_CFLAGS += -I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/usr/include/wifi-manager -I$(@D)
WIFI_MANAGER_DEMO_LDFLAGS = $(TARGET_LDFLAGS)
WIFI_MANAGER_DEMO_LDFLAGS += -L$(TARGET_DIR)/usr/lib/ -lpthread -lwpa_client -lwifimg

define WIFI_MANAGER_DEMO_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" CFLAGS="$(WIFI_MANAGER_DEMO_CFLAGS)" \
		LDFLAGS="$(WIFI_MANAGER_DEMO_LDFLAGS)" -C $(@D) all
endef

define WIFI_MANAGER_DEMO_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/wifi_connect_ap $(TARGET_DIR)/usr/bin/wifi_connect_ap
	$(INSTALL) -D -m 0755 $(@D)/wifi_scan_results $(TARGET_DIR)/usr/bin/wifi_scan_results
	$(INSTALL) -D -m 0755 $(@D)/wifi_connect_chinese_ap $(TARGET_DIR)/usr/bin/wifi_connect_chinese_ap
	$(INSTALL) -D -m 0755 $(@D)/wifi_list_networks $(TARGET_DIR)/usr/bin/wifi_list_networks
	$(INSTALL) -D -m 0755 $(@D)/wifi_on_off_test $(TARGET_DIR)/usr/bin/wifi_on_off_test
	$(INSTALL) -D -m 0755 $(@D)/wifi_remove_network $(TARGET_DIR)/usr/bin/wifi_remove_network
	$(INSTALL) -D -m 0755 $(@D)/wifi_stop_restart_scan_test $(TARGET_DIR)/usr/bin/wifi_stop_restart_scan_test
	$(INSTALL) -D -m 0755 $(@D)/wifi_add_network $(TARGET_DIR)/usr/bin/wifi_add_network
	$(INSTALL) -D -m 0755 $(@D)/wifi_connect_ap_with_netid $(TARGET_DIR)/usr/bin/wifi_connect_ap_with_netid
	$(INSTALL) -D -m 0755 $(@D)/wifi_get_netid $(TARGET_DIR)/usr/bin/wifi_get_netid
	$(INSTALL) -D -m 0755 $(@D)/wifi_longtime_test $(TARGET_DIR)/usr/bin/wifi_longtime_test
	$(INSTALL) -D -m 0755 $(@D)/wifi_remove_all_networks $(TARGET_DIR)/usr/bin/wifi_remove_all_networks
endef

$(eval $(generic-package))