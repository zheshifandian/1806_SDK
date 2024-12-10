include $(TOPDIR)/rules.mk

PKG_NAME:=liblocalhttp
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/liblocalhttp
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=localhttp
  DEPENDS:=+libuci +libcurl +libwebsockets +libc
endef

define Package/liblocalhttp/description
test for liblocalhttp set up
endef

define Build/Prepare
    mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

CFLAGS += $(FPIC)

define Build/Compile
    $(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(CFLAGS)" \
		all
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/liblocalhttp.so $(1)/usr/lib/
endef

define Package/liblocalhttp/install
    $(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/liblocalhttp.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,liblocalhttp))
