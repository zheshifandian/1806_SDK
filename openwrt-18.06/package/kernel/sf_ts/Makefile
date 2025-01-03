#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sf_ts
PKG_RELEASE:=1

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/sf_ts
  SUBMENU:=Network Devices
  TITLE:=siflower traffic statistics
  DEPENDS:=+kmod-sf_gmac
  FILES:=$(PKG_BUILD_DIR)/sf_ts.ko
  AUTOLOAD:=$(call AutoLoad,61,sf_ts,1)
  KCONFIG:=
endef

EXTRA_KCONFIG:= CONFIG_SFAX8_TS=m

ifdef  CONFIG_PACKAGE_kmod-sf_hnat
NOSTDINC_FLAGS += -DCONFIG_SFAX8_HNAT_DRIVER
endif


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ../siflower_include/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(CP) ../siflower_include/* $(PKG_BUILD_DIR)/
	$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		ARCH="$(LINUX_KARCH)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		SUBDIRS="$(PKG_BUILD_DIR)" \
		NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
		$(EXTRA_KCONFIG)  \
		modules
endef

define KernelPackage/sf_ts/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_BIN) ./files/tscript $(1)/usr/bin/
	$(INSTALL_BIN) ./files/ts_init $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/sidefault $(1)/etc/config/
endef

$(eval $(call KernelPackage,sf_ts))
