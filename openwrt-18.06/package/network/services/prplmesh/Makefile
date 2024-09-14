include $(TOPDIR)/rules.mk

PKG_NAME:=prplmesh
PKG_VERSION:=3.0.2
PKG_RELEASE:=1
PKG_REV:=$(PKG_VERSION)

PKG_SOURCE:=prplMesh-$(PKG_REV).tar.gz
PKG_SOURCE_URL:=https://gitlab.com/prpl-foundation/prplmesh/prplMesh/-/archive/$(PKG_REV)
PKG_HASH:=199ad22332adc3eb958b1c1fff5573ce6a0f30652a917615632defba07470196
PKG_BUILD_DIR:=$(BUILD_DIR)/prplMesh-$(PKG_REV)

#	CONFIG_WIRELESS_STA \
#	CONFIG_TARGET_intel_mips \
PKG_CONFIG_DEPENDS:= \
	CONFIG_PACKAGE_libamxb

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/prplmesh
	SECTION:=net
	CATEGORY:=Network
	TITLE:=prplmesh
	URL:=https://gitlab.com/prpl-foundation/prplmesh/prplMesh/-/blob/master/README.md
	MAINTAINER:=prplfoundation
	DEPENDS:= \
		+libstdcpp \
		+libpthread \
		+librt \
		+libjson-c \
		+libreadline \
		+libopenssl \
		+libncursesw \
		+libamxb \
		+libamxc \
		+libamxd \
		+libamxo \
		+libamxp \
		+libnl \
		+uci \
		+ubus \
		+ubox \
		+ebtables \
		+hostapd-utils \
		+liblzo \
		+libbz2 \
		+mod-amxb-ubus \
		+@BUSYBOX_CONFIG_ADDUSER \
		+@BUSYBOX_CONFIG_FEATURE_ADDUSER_LONG_OPTIONS \
		+@BUSYBOX_CONFIG_GETOPT \
		+@BUSYBOX_CONFIG_FEATURE_GETOPT_LONG
#	intel_mips specific dependencies:
#	DEPENDS+= \
#		+TARGET_intel_mips:dwpal_6x-uci \
#		+TARGET_intel_mips:iwlwav-hostap-uci \
#		+TARGET_intel_mips:libsafec3 \
#		+TARGET_intel_mips:bridge-ugw
#	non-intel_mips dependencies:
#	DEPENDS+= \
#		+!TARGET_intel_mips:hostapd-openssl \
#		+!TARGET_intel_mips:hostapd-utils \
#		+!TARGET_intel_mips:wpa-supplicant-openssl
#	The ambiorix library is optional, select it if libamxb is selected:
#	DEPENDS+= +PACKAGE_libamxb:libamxb \
#		+PACKAGE_libamxb:libamxc \
#		+PACKAGE_libamxb:libamxd \
#		+PACKAGE_libamxb:libamxo \
#		+PACKAGE_libamxb:libamxp \
#		+PACKAGE_libamxb:mod-amxb-ubus
endef

#define Package/prplmesh/config
#	source "$(SOURCE)/Config.in"
#endef

CMAKE_SOURCE_DIR:=
CMAKE_OPTIONS+= \
	-DTARGET_PLATFORM=openwrt \
	-DCMAKE_INSTALL_PREFIX=/opt/prplmesh \
	-DPLATFORM_BUILD_DIR=$(BUILD_DIR) \
	-DPLATFORM_STAGING_DIR=$(STAGING_DIR) \
	-DPLATFORM_INCLUDE_DIR=$(STAGING_DIR)/usr/include \
	-DBUILD_TESTS=OFF \
	-DCMAKE_BUILD_TYPE=Debug \
	$(if $(CONFIG_PACKAGE_libamxb), -DENABLE_NBAPI=ON) \
	-H. -B./build

ifeq ($(wildcard $(PKG_BUILD_DIR)/.source_dir),)
	CMAKE_OPTIONS += -DPRPLMESH_REVISION=$(PKG_REV)
endif

ifeq ($(CONFIG_TARGET_intel_mips),y)
	CMAKE_OPTIONS+= -DBWL_TYPE=DWPAL \
		-DTARGET_PLATFORM_TYPE=ugw \
		-DCMAKE_FIND_ROOT_PATH="${STAGING_DIR}/opt/intel;${CMAKE_FIND_ROOT_PATH}"
else
	CMAKE_OPTIONS+= -DBWL_TYPE=NL80211 \
		-DTARGET_PLATFORM_TYPE=turris-omnia
endif

Build/Compile:=cmake --build $(PKG_BUILD_DIR)/build -- $(MAKE_INSTALL_FLAGS) install -j
Build/Install:=
Build/Clean:=cd $(PKG_BUILD_DIR) && rm -rf .built .configured_* .prepared_* build/ ipkg-* || true

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/opt/prplmesh/lib/libbml* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/opt/prplmesh/include/beerocks/bml $(1)/usr/include/
endef

define Package/prplmesh/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_DIR) $(1)/opt/prplmesh
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DIR) $(1)/lib/tcl8.4
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) ./files/etc/init.d/* $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/etc/uci-defaults/* $(1)/etc/uci-defaults/
	$(CP) $(PKG_INSTALL_DIR)/opt/prplmesh/bin $(1)/opt/prplmesh/
	$(CP) $(PKG_INSTALL_DIR)/opt/prplmesh/scripts $(1)/opt/prplmesh/
	$(CP) $(PKG_INSTALL_DIR)/opt/prplmesh/lib/*.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/opt/prplmesh/share $(1)/opt/prplmesh/
	$(CP) $(PKG_INSTALL_DIR)/opt/prplmesh/config $(1)/opt/prplmesh/
	$(CP) ./files/etc/libexpect5.45.4.so $(1)/lib
	$(CP) ./files/etc/libtcl8.4.so $(1)/lib
	$(CP) ./files/etc/tcl8.4/* $(1)/lib/tcl8.4/
	$(INSTALL_BIN) ./files/etc/conf_update.sh $(1)/opt/prplmesh/scripts/
	$(INSTALL_BIN) ./files/etc/expect $(1)/bin/
	$(INSTALL_BIN) ./files/etc/wifi_reload $(1)/bin/
endef

$(eval $(call BuildPackage,prplmesh))
