#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=siwifitools
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/siwifitools
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=siwifi wifi debug tools
  MAINTAINER:=Robert chang <robert.chang@siflower.com.cn>
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -Wno-unused-variable -g" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/siwifitools/install
	$(INSTALL_DIR) $(1)/bin $(1)/lib/firmware/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gendesc $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gendiags $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/genvcd $(1)/bin/
	$(INSTALL_BIN) ./files/siwifi_umh.sh $(1)/bin/
ifdef CONFIG_TARGET_siflower_sf19a28_fullmask
	$(INSTALL_DATA) ./files/sf19a28/*.txt $(1)/lib/firmware/
else
ifdef CONFIG_TARGET_siflower_sf19a28_mpw1
	$(INSTALL_DATA) ./files/sf19a28/*.txt $(1)/lib/firmware/
else
	$(INSTALL_DATA) ./files/sf16a18/*.txt $(1)/lib/firmware/
endif
endif
endef

$(eval $(call BuildPackage,siwifitools))
