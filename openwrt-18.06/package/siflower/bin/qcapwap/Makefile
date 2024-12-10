# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=qcapwap
PKG_VERSION:=1.0.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/siwifi_version.mk

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/qcapwap
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Implementation of the CAPWAP protocol
  DEPENDS:= +libpthread +libuci +libjson-c +libevent2-core
endef

define Package/qcapwap_ac
  $(Package/qcapwap)
  TITLE:=Opencapwap Access Controller(AC)
  DEPENDS+=qcapwap
endef

define Package/qcapwap_wtp
  $(Package/qcapwap)
  TITLE:=Opencapwap Wireless Terminator Point(WTP)
endef

define Package/qcapwap/description
  Open source implementation of the CAPWAP protocol according to RFC 4515 and RFC 4516.
endef

define Package/qcapwap/config
	source "$(SOURCE)/Config.in"
endef

TARGET_CFLAGS += -I${STAGING_DIR}/usr/include/libnl3
TARGET_CFLAGS += -I$(PKG_BUILD_DIR)

SF_DOWNLOAD_BIN=/usr/sbin/wtp_do_download
SF_UPDATE_BIN=/usr/sbin/wtp_do_update
TARGET_CFLAGS += -D'SF_UPDATE_BIN=\"$(SF_UPDATE_BIN)\"' -D'SF_DOWNLOAD_BIN=\"$(SF_DOWNLOAD_BIN)\"'

ifeq ($(CONFIG_QCAPWAP_DEBUG),y)
TARGET_CFLAGS += -DCW_DEBUGGING -g3 -O0
TARGET_LDFLAGS += -rdynamic
endif

define Package/qcapwap_ac/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/capwap
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/AC $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/WUM $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/update/wtp_update.sh $(1)/usr/sbin/wtp_update
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/check_ac.sh $(1)/etc/capwap
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/ap_groups $(1)/etc/config
ifneq ($(CONFIG_PACKAGE_qcapwap_wtp),y)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ac_server.sh $(1)/etc/init.d/ac_server
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ac_import.sh $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/acserver.hotplug $(1)/etc/hotplug.d/iface/96-acserver
endif
endef

define Package/qcapwap_wtp/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/capwap
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/WTP $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/check_wtp.sh $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/update/do_update.sh $(1)$(SF_UPDATE_BIN)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/update/do_download.sh $(1)$(SF_DOWNLOAD_BIN)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hostapd_config.sh $(1)/usr/sbin/hostapd_config
ifneq ($(CONFIG_PACKAGE_qcapwap_ac),y)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wtp_server.sh $(1)/etc/init.d/wtp_server
endif
endef

$(eval $(call BuildPackage,qcapwap))
$(eval $(call BuildPackage,qcapwap_ac))
$(eval $(call BuildPackage,qcapwap_wtp))
