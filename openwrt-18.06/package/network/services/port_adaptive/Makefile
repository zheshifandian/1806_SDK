
include $(TOPDIR)/rules.mk

PKG_NAME:=port_adaptive
PKG_VERSION:=1
PKG_RELEASE:=2
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Package/port_adaptive
  SECTION:=net
  CATEGORY:=Network
  TITLE:=WAN/LAN port auto adapt demo
  DEPENDS:= +libuci
endef

define Package/port_adaptive/description
 WAN/LAN auto adapt demo
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -g" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Build/Configure
endef

define Package/port_adaptive/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/send $(1)/bin
	$(INSTALL_BIN) ./files/auto_adapt.sh $(1)/bin

ifdef CONFIG_PACKAGE_luci2-siflower
	echo "NOW install port_adaptive for luci2 webpage"
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/port_adaptive $(1)/etc/init.d
endif

ifdef CONFIG_PACKAGE_luci-mod-admin-full-siflower
	echo "NOW install port_adaptive for luci webpage"
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/new_siwifi/senior_user
	$(INSTALL_DATA) ./files/adapt.htm $(1)/usr/lib/lua/luci/view/new_siwifi/senior_user
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) ./files/rc.local $(1)/etc
endif

endef

$(eval $(call BuildPackage,port_adaptive))
