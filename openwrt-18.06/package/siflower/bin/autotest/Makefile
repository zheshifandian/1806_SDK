#
# Copyright (C) 2016-2019 qin.xia@siflower.com.cn
#
# sifiower Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=autotest
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/autotest
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=autotest
  MAINTAINER:=qin.xia <qin.xia@siflower.com.cn>
  DEPENDS:=+libc +librt +libubus +libuci +libcurl +libjson-c +libpcap +libopenssl +libubox
endef

define Package/autotest/description
autotest.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += \
	-I$(LINUX_DIR)/include \
	-I$(LINUX_DIR)/drivers/char

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -Wno-unused-but-set-variable -Wno-unused-function" \
		LDFLAGS="$(TARGET_LDFLAGS) -lc -luci -lcurl -ljson-c -lpcap -lcrypto -lubox"
endef

define Package/autotest/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/autotest_init $(1)/usr/sbin
	$(INSTALL_BIN) ./files/autotest_init $(1)/etc/init.d/
endef


$(eval $(call BuildPackage,autotest))
