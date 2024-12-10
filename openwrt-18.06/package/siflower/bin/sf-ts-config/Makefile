#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sf-ts-config
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/sf-ts-config
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=sf-ts-config for api sf-ts-config()
  DEPENDS:=+libuci
endef

define Package/sf-ts-config/description
sf-ts config tool
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Package/sf-ts-config/conffiles
/etc/sf-ts-cfg
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -Wno-unused-but-set-variable -Wno-unused-function" \
		LDFLAGS="$(TARGET_LDFLAGS) -lm -luci"
endef

define Package/sf-ts-config/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tscfg $(1)/usr/bin/
	$(INSTALL_BIN) ./files/auto-check-ts-version.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/first-check-ts-file.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/pctl_upgrade $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_BIN) ./files/tsconfig $(1)/etc/config/tsconfig
endef

$(eval $(call BuildPackage,sf-ts-config))
