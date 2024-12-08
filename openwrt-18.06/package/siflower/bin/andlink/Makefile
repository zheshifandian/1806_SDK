#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=andlink
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/andlink
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=connect machine to cmcc andlink
  MAINTAINER:=chang li <chang.li@siflower.com.cn>
  DEPENDS:=+libc +libpthread +libubus +libcoap +libuci +libjson-c
endef

define Package/andlink/description
Connect machine to cmcc andlink
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp ./src/* $(PKG_BUILD_DIR) -r
	cp ./file $(PKG_BUILD_DIR) -r
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -Wno-unused-but-set-variable -Wno-unused-function -D P2P" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(PKG_BUILD_DIR) -lc -lubus -lubox -lcoap-2 -luci -ljson-c"
endef

define Package/andlink/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/andlink $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/file/wifi_control.sh $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/file/andlink $(1)/etc/config/
endef

$(eval $(call BuildPackage,andlink))
