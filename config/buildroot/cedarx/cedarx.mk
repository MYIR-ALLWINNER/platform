################################################################################
#
# cedarx
#
################################################################################
CEDARX_SITE_METHOD = local
CEDARX_SITE = $(PLATFORM_PATH)/../../framework/cedarx
CEDARX_LICENSE = GPLv2+, GPLv3+
CEDARX_LICENSE_FILES = Copyright COPYING
CEDARX_INSTALL_TARGET = YES
CEDARX_INSTALL_STAGING = YES
CEDARX_AUTORECONF = YES
#CEDARX_GETTEXTIZE = YES
CEDARX_CONF_OPTS += CFLAGS="-D__ENABLE_ZLIB__"
CEDARX_CONF_OPTS += CPPFLAGS="-D__ENABLE_ZLIB__"
CEDARX_CONF_OPTS += LDFLAGS="-L$(TARGET_DIR)/usr/lib"

CEDARX_DEPENDENCIES += \
	zlib \
	openssl

define CEDARX_CONFIGURE_CMDS
	(cd $(@D); \
	$(TARGET_CONFIGURE_OPTS) \
	$(TARGET_CONFIGURE_ARGS) \
	CONFIG_SITE=/dev/null \
	./configure \
		--target=$(GNU_TARGET_NAME) \
		--host=$(GNU_TARGET_NAME) \
		--build=$(GNU_HOST_NAME) \
		--prefix=$(TARGET_DIR)/usr \
		--exec-prefix=$(TARGET_DIR)/usr \
		--sysconfdir=$(TARGET_DIR)/etc \
		--program-prefix="" \
		--disable-gtk-doc \
		--disable-gtk-doc-html \
		--disable-doc \
		--disable-docs \
		--disable-documentation \
		--with-xmlto=no \
		--with-fop=no \
		--disable-dependency-tracking \
		--enable-ipv6 \
		$(DISABLE_NLS) \
		$(SHARED_STATIC_LIBS_OPTS) \
		$(CEDARX_CONF_OPTS) \
	)
endef

define CEDARX_BUILD_CMDS
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libaw* $(STAGING_DIR)/usr/lib
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libadecoder.so $(STAGING_DIR)/usr/lib
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libaencoder.so $(STAGING_DIR)/usr/lib
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libsubdecoder.so $(STAGING_DIR)/usr/lib
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libsalsa.so $(STAGING_DIR)/usr/lib
	$(MAKE) -C $(@D)
	$(MAKE) -C $(@D) install
endef

define CEDARX_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/cdx_config.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/external/include/adecoder/*.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/external/include/sdecoder/*.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/external/include/alsa/  $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/xplayer/include/*.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/libcore/base/include/*.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/libcore/base/include/*.i $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/libcore/muxer/include/*.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/libcore/parser/include/*.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/libcore/stream/include/*.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/libcore/common/iniparser/*.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/libcore/common/plugin/*.h $(STAGING_DIR)/usr/include/cedarx
	cp -rf $(@D)/libcore/playback/include/*.h $(STAGING_DIR)/usr/include/cedarx
endef

#fix me
define CEDARX_INSTALL_TARGET_CMDS
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libaw* $(TARGET_DIR)/usr/lib
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libadecoder.so $(TARGET_DIR)/usr/lib
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libaencoder.so $(TARGET_DIR)/usr/lib
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libsubdecoder.so $(TARGET_DIR)/usr/lib
	cp -rf $(@D)/external/lib32/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/libsalsa.so $(TARGET_DIR)/usr/lib
	cp -rf $(@D)/config/${LICHEE_PRODUCT}_cedarx.conf $(TARGET_DIR)/etc/cedarx.conf
endef

$(eval $(autotools-package))
