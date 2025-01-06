#
# Copyright (C) 2013 Jo-Philipp Wich <jow@openwrt.org>
#
# Licensed under the Apache License, Version 2.0.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=webubus
PKG_VERSION:=1.0
PKG_MAINTAINER:=Jo-Philipp Wich <jow@openwrt.org>

PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/webubus
  SECTION:=webubus
  CATEGORY:=WebUbus
  TITLE:=WebUbus API
  DEPENDS:=+rpcd +rpcd-mod-iwinfo +uhttpd +uhttpd-mod-ubus +libubox +libubus
endef

define Package/webubus/description
 Siflower webubus interface api.
endef

define Package/webubus/install
	$(INSTALL_DIR) $(1)/usr/lib/rpcd
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/networknew/network.so $(1)/usr/lib/rpcd/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wirelessnew/wireless.so $(1)/usr/lib/rpcd/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/advancenew/advance.so $(1)/usr/lib/rpcd/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/systemnew/system.so $(1)/usr/lib/rpcd/
endef

$(eval $(call BuildPackage,webubus))
