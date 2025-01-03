include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk

PKG_NAME:=tcontrol
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

define Package/tcontrol
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=temperature ccontrol protect
  MAINTAINER:=ming.guang <ming.guang@siflower.com.cn>
  DEPENDS:=+libpthread
endef

define Package/tcontrol/description
Let the chip work in a safe temperature range
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
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS) -lpthread"
endef

define Package/tcontrol/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tcontrol $(1)/usr/bin/
	$(INSTALL_BIN) ./files/tempservice $(1)/etc/init.d/
ifdef CONFIG_SF19A28_COOLING_FIN_NO
	$(INSTALL_BIN) ./files/nofin/temperature_lb_control.ini $(1)/lib/firmware/
	$(INSTALL_BIN) ./files/nofin/temperature_hb_control.ini $(1)/lib/firmware/
else
	$(INSTALL_BIN) ./files/temperature_lb_control.ini $(1)/lib/firmware/
	$(INSTALL_BIN) ./files/temperature_hb_control.ini $(1)/lib/firmware/
endif
endef

$(eval $(call BuildPackage,tcontrol))
