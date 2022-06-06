################################################################################
#
# libcedare lib
#
################################################################################
LIBCEDARC_SITE_METHOD = local
LIBCEDARC_SITE = $(PLATFORM_PATH)/../../framework/libcedarc
LIBCEDARC_LICENSE = GPLv2+, GPLv3+
LIBCEDARC_LICENSE_FILES = Copyright COPYING
LIBCEDARC_INSTALL_TARGET = YES
LIBCEDARC_INSTALL_STAGING = YES
LIBCEDARC_AUTORECONF = YES
#LIBCEDARC_GETTEXTIZE = YES

LIBCEDARC_CONF_OPTS += CFLAGS="-DCONF_KERNEL_VERSION_3_10 -share"
LIBCEDARC_CONF_OPTS += CPPFLAGS="-DCONF_KERNEL_VERSION_3_10"

define LIBCEDARC_CONFIGURE_CMDS
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
	)
endef

define LIBCEDARC_BUILD_CMDS
	cp -rf $(@D)/library/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/* $(STAGING_DIR)/usr/lib
	$(MAKE) -C $(@D)
	$(MAKE) -C $(@D) install
endef

define LIBCEDARC_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/libcedarc
	cp -rf $(@D)/include/*.h $(STAGING_DIR)/usr/include/libcedarc
endef

#fix me
define LIBCEDARC_INSTALL_TARGET_CMDS
	cp -rf $(@D)/library/$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)/*.so $(TARGET_DIR)/usr/lib
endef

$(eval $(autotools-package))
