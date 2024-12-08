#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk
include $(INCLUDE_DIR)/package.mk


PKG_NAME:=siflower-common
PKG_VERSION:=1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/siflower-common/Default
  SECTION:=sfcommon
  CATEGORY:=Utilities
  TITLE:=siflowr-common
  SUBMENU:=siflower-common
endef

define Build/Compile
endef

define Package/hnat-support
$(call Package/siflower-common/Default)
  TITLE:=hnat support files
endef

define Package/hnat-support/config
	if PACKAGE_hnat-support
	config PACKAGE_hnat-support_hotplug
		bool "hnat - hotplug files to update hnat wan/lan info"
		default "n"
	config PACKAGE_hnat-support_script
		bool "hnat - debug scripts"
		default "n"
	config PACKAGE_hnat-support_addifname
		bool "hnat - update wan/lan ifname to debugfs"
		default "n"
	endif
endef

define Package/hnat-support/install

ifdef CONFIG_PACKAGE_hnat-support_hotplug
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(CP) ./hnat/etc/hotplug.d/iface/05-updatehnatlan $(1)/etc/hotplug.d/iface/
	$(CP) ./hnat/etc/hotplug.d/iface/80-updatewan $(1)/etc/hotplug.d/iface/
endif

ifdef CONFIG_PACKAGE_hnat-support_script
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) ./hnat/usr/bin/hnat_read.sh $(1)/usr/bin/
	$(CP) ./hnat/usr/bin/hnat_enable.sh $(1)/usr/bin/
endif

ifdef CONFIG_PACKAGE_hnat-support_addifname
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) ./hnat/usr/bin/hnat_update_interface.sh $(1)/usr/bin/
endif

endef

define Package/systems
$(call Package/siflower-common/Default)
  TITLE:=system files
endef

define Package/systems/config
	if PACKAGE_systems
	config PACKAGE_systems_sys-reset
		bool "sys - sf_reset.sh"
		default "n"
	config PACKAGE_systems_sys-clean_log
		bool "sys - clean_log"
		default "y"
	config PACKAGE_systems_sys-upgrade
		bool "sys - platform.sh"
		default "n"
	config PACKAGE_systems_sys-auto-ota
		bool "sys - auto_ota"
		default "n"
	config PACKAGE_systems_sys-otaupgrade
		bool "sys - otaupgrade"
		default "n"
	config PACKAGE_systems_sys-cpufreq
		bool "sys - set cpu frequence"
		default "n"
	config PACKAGE_systems_sys-fixtime
		bool "sys - fix system time"
		default "n"
	config PACKAGE_systems_sys-pwrmgmt
		bool "sys - set power maneagement"
		default "n"
	config PACKAGE_systems_sys-ota
		bool "sys - ota"
		default "n"
	endif
endef

define Package/systems/install

ifdef CONFIG_PACKAGE_systems_sys-reset
	$(INSTALL_DIR) $(1)/bin
	$(CP) ./system/bin/sf_reset.sh $(1)/bin/
endif

ifdef CONFIG_PACKAGE_systems_sys-reboot
	$(INSTALL_DIR) $(1)/bin
	$(CP) ./system/bin/sf_reboot.sh $(1)/bin/
endif

ifdef CONFIG_PACKAGE_systems_sys-clean_log
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) ./system/usr/sbin/clean_log $(1)/usr/sbin/
endif

ifdef CONFIG_PACKAGE_systems_sys-upgrade
	$(INSTALL_DIR) $(1)/lib/upgrade/
	$(CP) ./upgrade/lib/upgrade/platform.sh $(1)/lib/upgrade/
endif

ifdef CONFIG_PACKAGE_systems_sys-otaupgrade
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) ./upgrade/usr/bin/otaupgrade $(1)/usr/bin/
endif

ifdef CONFIG_PACKAGE_systems_sys-auto-ota
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) ./upgrade/usr/sbin/auto_ota $(1)/usr/sbin/
endif

ifdef CONFIG_PACKAGE_systems_sys-cpufreq
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./system/bin/set_cpufreq $(1)/usr/sbin/
	$(CP) ./system/etc/init.d/set_default_cpufreq $(1)/etc/init.d/
endif

ifdef CONFIG_PACKAGE_systems_sys-fixtime
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./system/etc/init.d/sysfixtime $(1)/etc/init.d/
endif

ifdef CONFIG_PACKAGE_systems_sys-pwrmgmt
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) ./system/usr/sbin/set_pwrmgmt $(1)/usr/sbin/
endif

ifdef CONFIG_PACKAGE_systems_sys-ota
	$(INSTALL_DIR) $(1)/usr/sbin
ifeq ($(CONFIG_VERSION_MANUFACTURER),JUOVI)
	$(CP) ./ota/usr/sbin/ota_juovi $(1)/usr/sbin/ota
else
	$(CP) ./ota/usr/sbin/ota $(1)/usr/sbin/
endif
endif

endef

define Package/configs
$(call Package/siflower-common/Default)
  TITLE:=config setting files
endef

define Package/configs/config
	if PACKAGE_configs
	config PACKAGE_configs_conf-check
		bool "conf - config_check.sh"
		default "n"
	config PACKAGE_configs_conf-base
		bool "conf - basic configs"
		default "n"
	config PACKAGE_configs_conf-uci
		bool "conf - uci support"
		default "n"
	config PACKAGE_configs_conf-01network
		bool "conf - 01_network"
		default "n"
	endif
endef

define Package/configs/install

ifdef CONFIG_PACKAGE_configs_conf-check
	$(INSTALL_DIR) $(1)/bin
	$(CP) ./base-config/bin/config_check.sh $(1)/bin/
endif

ifdef CONFIG_PACKAGE_configs_conf-base
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./base-config/etc/sysupgrade_except.conf $(1)/etc/
	$(CP) ./base-config/etc/config/* $(1)/etc/config/
	if [ "$(shell echo $(PROFILE) | grep "FLASH")" != "" ] || [ "$(shell echo $(PROFILE) | grep "86V")" != "" ]; then\
		rm $(1)/etc/config/basic_setting; \
		rm $(1)/etc/config/timelist; \
		rm $(1)/etc/config/notify; \
		rm $(1)/etc/config/qos_cfg; \
		rm $(1)/etc/config/samba; \
		rm $(1)/etc/config/wifi_info; \
		rm $(1)/etc/sysupgrade_except.conf; \
	fi
endif

ifdef CONFIG_PACKAGE_configs_conf-uci
	$(INSTALL_DIR) $(1)/lib/functions
	$(CP) ./base-config/lib/functions/uci-defaults-new.sh $(1)/lib/functions/
endif

ifdef CONFIG_PACKAGE_configs_conf-01network
	$(INSTALL_DIR) $(1)/etc/
	$(INSTALL_DIR) $(1)/etc/board.d/
	$(CP) ./base-config/etc/board.d/01_network $(1)/etc/board.d/
endif

endef

define Package/networks
$(call Package/siflower-common/Default)
  TITLE:=scprits about ethernets
endef

define Package/networks/config
	if PACKAGE_networks
	config PACKAGE_configs_net-netclash
		bool "net - netclash"
		default "n"
	config PACKAGE_configs_net-speed
		bool "net - speed"
		default "n"
	config PACKAGE_configs_net-wan
		bool "net - wan speed and status"
		default "n"
	config PACKAGE_configs_net-check
		bool "net - check_net"
		default "n"
	config PACKAGE_configs_net-dhcpserver
		bool "net - dhcp server status"
		default "n"
	endif
endef

define Package/networks/install

ifdef CONFIG_PACKAGE_configs_net-netclash
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) ./networks/usr/sbin/netclash $(1)/usr/sbin/
endif

ifdef CONFIG_PACKAGE_configs_net-speed
	$(INSTALL_DIR) $(1)/sbin
	$(CP) ./networks/sbin/speed $(1)/sbin/
endif

ifdef CONFIG_PACKAGE_configs_net-wan
	$(INSTALL_DIR) $(1)/bin
	$(CP) ./networks/bin/wanLinkStatus $(1)/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) ./networks/etc/init.d/wan_speed $(1)/etc/init.d/
endif

ifdef CONFIG_PACKAGE_configs_net-check
	$(INSTALL_DIR) $(1)/sbin
	$(CP) ./networks/sbin/check_net $(1)/sbin/
endif

ifdef CONFIG_PACKAGE_configs_net-dhcpserver
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./dhcpserver/etc/init.d/get_dhcpserver_status $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/bin
	$(CP) ./dhcpserver/bin/dhcpserver_status.sh $(1)/bin/
endif

endef

define Package/fpga-scripts
$(call Package/siflower-common/Default)
  TITLE:=files to support fpga
endef

define Package/fpga-scripts/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) -r ./fpga-scripts/usr/bin/* $(1)/usr/bin/
endef

define Package/si-cron
$(call Package/siflower-common/Default)
  TITLE:=siflower cron event
endef

define Package/si-cron/install
	$(INSTALL_DIR) $(1)/etc/crontabs/
	if [ "$(CONFIG_PACKAGE_systems_sys-clean_log)" = "y" ]; then\
	cat ./si-cron/log_cron >> $(1)/etc/crontabs/root;\
	fi
	if [ "$(CONFIG_PACKAGE_kmod-sf-ts)" = "y" ]; then\
	cat ./si-cron/sf_ts_cron >> $(1)/etc/crontabs/root;\
	fi
	if [ "$(CONFIG_PACKAGE_systems_sys-auto-ota)" = "y" ]; then\
	cat ./si-cron/auto_ota_cron >> $(1)/etc/crontabs/root;\
	fi
endef

define Package/online-warn
$(call Package/siflower-common/Default)
  TITLE:=new wifi device online warning
endef

define Package/online-warn/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) ./online-warning/online-warn $(1)/usr/sbin/
	$(CP) ./online-warning/siwarning $(1)/usr/sbin/
	$(CP) ./online-warning/online-warn-init $(1)/etc/init.d/
endef

define Package/parents-control
$(call Package/siflower-common/Default)
  TITLE:=parents-control
endef

define Package/parents-control/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/hotplug.d/ntp
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_BIN) ./parents-control/pctl $(1)/usr/sbin/
	$(INSTALL_BIN) ./parents-control/init_pctl $(1)/etc/init.d/
	$(INSTALL_BIN) ./parents-control/sync_acl $(1)/etc/hotplug.d/ntp/
	$(INSTALL_BIN) ./parents-control/60-speedlimit $(1)/etc/hotplug.d/iface/
endef

define Package/aclscript
$(call Package/siflower-common/Default)
TITLE:=aclscript
endef

define Package/aclscript/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./aclscript/bin/aclscript $(1)/usr/sbin/
	$(CP) ./aclscript/etc/init.d/acl $(1)/etc/init.d/
endef

define Package/devlist
$(call Package/siflower-common/Default)
TITLE:=devlist operation scripts
endef

define Package/devlist/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) ./devlist/bin/devlistclean $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/sbin
	$(CP) ./devlist/sbin/init_devlist $(1)/sbin/
endef

define Package/sslcert
$(call Package/siflower-common/Default)
TITLE:=sslcert
endef

define Package/sslcert/install
	$(INSTALL_DIR) $(1)/etc/ssl/certs/
	$(CP) ./sslcert/etc/se_pub.key $(1)/etc/
	$(CP) ./sslcert/etc/pubkey.pem $(1)/etc/
	$(CP) ./sslcert/etc/ssl/certs/* $(1)/etc/ssl/certs/
endef

define Package/wifis
$(call Package/siflower-common/Default)
TITLE:=wifi files
endef

define Package/wifis/config
	if PACKAGE_wifis
	config PACKAGE_configs_wifi-gwifi
		bool "wifi - gwifi filse"
		default "n"
	config PACKAGE_configs_wifi-check_wds
		bool "wifi - check wds"
		default "n"
	config PACKAGE_configs_wifi-wlanctl
		bool "wifi - wlan control"
		default "n"
	endif
endef

define Package/wifis/install

ifdef CONFIG_PACKAGE_configs_wifi-gwifi
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) ./wifis/usr/sbin/gwifi $(1)/usr/sbin/
endif

ifdef CONFIG_PACKAGE_configs_wifi-check_wds
	$(INSTALL_DIR) $(1)/sbin
	$(CP) ./wifis/sbin/check_wds $(1)/sbin/
	$(CP) ./wifis/sbin/alter_lan $(1)/sbin/
	$(CP) ./wifis/sbin/wds_end_setting.lua $(1)/sbin/
endif

ifdef CONFIG_PACKAGE_configs_wifi-wlanctl
	$(INSTALL_DIR) $(1)/etc/hotplug.d/net
	$(CP) ./wifis/etc/hotplug.d/net/01-wlanctl $(1)/etc/hotplug.d/net/
endif

endef

define Package/ssst_replace
$(call Package/siflower-common/Default)
TITLE:=ssst_replace.sh
endef

define Package/ssst_replace/install
	$(INSTALL_DIR) $(1)/etc
	$(CP) ./ssst/etc/ssst_replace.sh $(1)/etc/
endef

define Package/86V_FAT_DHCP
$(call Package/siflower-common/Default)
TITLE:=dhcp.sh for 86V
endef

define Package/86V_FAT_DHCP/install
	$(INSTALL_DIR) $(1)/bin
	$(CP) ./base-config/bin/dhcp.sh $(1)/bin/
endef

$(eval $(call BuildPackage,hnat-support))
$(eval $(call BuildPackage,systems))
$(eval $(call BuildPackage,configs))
$(eval $(call BuildPackage,networks))
$(eval $(call BuildPackage,fpga-scripts))
$(eval $(call BuildPackage,si-cron))
$(eval $(call BuildPackage,online-warn))
$(eval $(call BuildPackage,parents-control))
$(eval $(call BuildPackage,aclscript))
$(eval $(call BuildPackage,devlist))
$(eval $(call BuildPackage,sslcert))
$(eval $(call BuildPackage,wifis))
$(eval $(call BuildPackage,ssst_replace))
$(eval $(call BuildPackage,86V_FAT_DHCP))
