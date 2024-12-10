#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=new_device_listen
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_DEPENDS:=openssl

include $(INCLUDE_DIR)/package.mk

define Package/new_device_listen
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=new_device_listen
  DEPENDS:=+libc +libuci +libcurl +libnet-1.2.x +libpcap +libubox
endef

define Package/new_device_listen/description
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
		CFLAGS="$(TARGET_CFLAGS) -Wall -Wno-unused-but-set-variable -Wno-unused-function" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(PKG_BUILD_DIR) -lc -luci -lcurl -lpcap -lnet -lubox"
endef


define Package/new_device_listen/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/new_device_listen $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/updatewldevlist.sh $(1)/usr/sbin
	$(INSTALL_BIN) ./files/device_listen $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/devlist $(1)/etc/config/
	$(INSTALL_BIN) ./files/wldevlist $(1)/etc/config/
	$(INSTALL_BIN) ./files/meshlist $(1)/etc/config/
	$(INSTALL_BIN) ./files/dev.sh $(1)/sbin/
	$(INSTALL_BIN) ./files/wan_detect.sh $(1)/sbin/
	$(INSTALL_BIN) ./files/easy_mesh.sh $(1)/sbin/
endef

$(eval $(call BuildPackage,new_device_listen))
