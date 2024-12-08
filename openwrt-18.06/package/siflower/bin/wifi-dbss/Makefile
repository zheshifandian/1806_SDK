#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=wifi-dbss
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/wifi-dbss/default
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=dual band single ssid.
  MAINTAINER:=Nevermore <nevermore.wang@siflower.com.cn>
ifndef CONFIG_PACKAGE_wifi-dbss-mod-curl
  DEPENDS:=+hostapd-common +libblobmsg-json +libubus +libjson-c 
else
  DEPENDS:=+hostapd-common +libblobmsg-json +libubus +libjson-c +libcurl
endif
endef

define Package/wifi-dbss
  $(Package/wifi-dbss/default)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/wifi-dbss/description
Advanced wifi: dual band single ssid, auto switch.
endef

define Build/Configure
endef

ifdef CONFIG_PACKAGE_wifi-dbss-mod-curl
CURL_FLAGS:=-lcurl -Dsiwifi_dbss_curl
endif

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -Werror" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		CURL_FLAGS="$(CURL_FLAGS)"
endef

define Package/wifi-dbss/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) ./files/advanced_wifi  $(1)/etc/init.d/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wifi-dbss $(1)/usr/sbin/wifi-dbss
endef

define Package/wifi-dbss-mod-curl
  $(Package/wifi-dbss/default)
  TITLE+=(Upload data to the server)
endef

$(eval $(call BuildPackage,wifi-dbss))
$(eval $(call BuildPackage,wifi-dbss-mod-curl))
