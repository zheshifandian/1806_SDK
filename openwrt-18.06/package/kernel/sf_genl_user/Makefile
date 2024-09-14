include $(TOPDIR)/rules.mk

PKG_NAME:=sf_genl_user
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/sf_genl_user
  CATEGORY:=Siflower
  TITLE:=Siflower Generic Netlink UBUS Interface
  DEPENDS:=+libnl +libnl-genl +libubus +libubox
endef

define Package/sf_genl_user/description
  Siflower Generic Netlink UBUS Interface
endef

define Build/Configure
endef

ifdef CONFIG_PACKAGE_kmod-dpns_tmu
TARGET_CFLAGS += -DCONFIG_SIFLOWER_GENL_TMU
endif

ifdef CONFIG_PACKAGE_kmod-dpns_mcast
TARGET_CFLAGS += -DCONFIG_SIFLOWER_GENL_MCAST
endif

ifdef CONFIG_PACKAGE_kmod-dpns_acl
TARGET_CFLAGS += -DCONFIG_SIFLOWER_GENL_ACL
endif

define Build/Compile
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ../sf_eth/dpns_genl/src/sf_genl.h        $(PKG_BUILD_DIR)/
	$(CP) ../sf_eth/dpns_l2/src/se_common.h        $(PKG_BUILD_DIR)/
	$(CP) ../sf_eth/dpns_tmu/src/sf_tmu_regs.h     $(PKG_BUILD_DIR)/
	$(CP) ../sf_eth/dpns_tmu/src/sf_tmu_genl.h     $(PKG_BUILD_DIR)/
	$(CP) ../sf_eth/dpns_mcast/src/se_mcast_cfg.h  $(PKG_BUILD_DIR)/
	$(CP) ../sf_eth/dpns_mcast/src/sf_genl_mcast.h  $(PKG_BUILD_DIR)/

ifdef CONFIG_PACKAGE_kmod-dpns_acl
	$(CP) ../sf_eth/dpns_acl/src/dpns_acl_msg.h	$(PKG_BUILD_DIR)/
endif

ifdef CONFIG_TARGET_siflower_sf21h8898_mpw0
	$(CP) ../sf_eth/dpns_genl/src/sf_genl_msg.h    $(PKG_BUILD_DIR)/
else
	$(CP) ../sf_eth/include/sf_genl_msg.h          $(PKG_BUILD_DIR)/
	$(CP) ../sf_eth/dpns_nat/src/dpns_nat_genl.h	$(PKG_BUILD_DIR)/
	$(CP) ../sf_eth/dpns_vlan/src/dpns_vlan_genl.h  $(PKG_BUILD_DIR)/
	$(CP) ../sf_eth/dpns_router/src/dpns_router_genl.h  $(PKG_BUILD_DIR)/
	$(CP) ../sf_eth/dpns_common/src/dpns_common_genl.h  $(PKG_BUILD_DIR)/
endif

	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -Wno-unused-but-set-variable -Wno-unused-function" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(PKG_BUILD_DIR) -lc -lubus -lubox -lnl-genl -lnl"
endef

define Package/sf_genl_user/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sf_genl_user $(1)/usr/sbin
	$(INSTALL_BIN) ./files/limit.sh $(1)/usr/sbin
	$(INSTALL_BIN) ./files/dev.sh $(1)/usr/sbin
	$(INSTALL_BIN) ./files/genl_init $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/devlist $(1)/etc/config/
endef

$(eval $(call BuildPackage,sf_genl_user))
