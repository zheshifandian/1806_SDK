#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=eth0_listen
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/eth0_listen
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=eth0_listen
  DEPENDS:=+libnl-tiny +ubusd +jshn +libubox
endef

define Package/eth0_listen/description
endef

TARGET_CFLAGS += $(FPIC)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef


define Build/Compile/eth0_listen
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(PKG_BUILD_DIR) -lnl-tiny -lubox"
endef

define Build/Compile
	$(Build/Compile/eth0_listen)
endef


define Package/eth0_listen/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/eth0_listen $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/et_listen $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,eth0_listen))
