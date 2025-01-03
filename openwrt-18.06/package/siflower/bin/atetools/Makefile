#
# Copyright @ 2024 Siflower
#
# sifiower Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ate_cmd
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/ate_cmd
SECTION:=utils
CATEGORY:=Utilities
TITLE:=atetools.
MAINTAINER:=Star jiang <star.jiang@siflower.com.cn>
DEPENDS:=+libc +libpthread +librt +libubus
endef

define Package/ate_cmd/description
atetest tool.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

#TARGET_CFLAGS += \
#	-I$(LINUX_DIR)/arch/mips/include \
#	-I$(LINUX_DIR)/include \
#	-I$(LINUX_DIR)/drivers/char

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -Wno-unused-but-set-variable -Wno-unused-function" \
		LDFLAGS="$(TARGET_LDFLAGS) -lpthread -lc -lm"
endef

define Package/ate_cmd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/atetools
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ate_cmd $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ate_server $(1)/usr/bin
	$(CP) ./files/*  $(1)/etc/atetools/
	$(CP) ./files/*  $(1)/usr/bin/
endef


$(eval $(call BuildPackage,ate_cmd))
