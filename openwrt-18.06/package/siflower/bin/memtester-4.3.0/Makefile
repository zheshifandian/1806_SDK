#
# Copyright (C) 2015-2018 neil.zhang@siflower.com.cn
#
# sifiower Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=memtester
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/memtester
SECTION:=utils
CATEGORY:=Utilities
TITLE:=memtester 4.3.0.
MAINTAINER:=Nevermore <nevermore.wang@siflower.com.cn>
endef

define Package/memtester/description
memtester 4.3.0
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
		CFLAGS="$(TARGET_CFLAGS) -Wall -Werror -mips32r2" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/memtester/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/memtester $(1)/bin/
	$(INSTALL_BIN) ./files/memtester-multi.sh  $(1)/bin/
endef


$(eval $(call BuildPackage,memtester))
