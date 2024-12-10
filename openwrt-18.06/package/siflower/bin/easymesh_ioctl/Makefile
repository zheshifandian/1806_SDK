include $(TOPDIR)/rules.mk

PKG_NAME:=easymesh_ioctl
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
KO_FILE_NAME="bin"

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/easymesh_ioctl
SECTION:=utils
CATEGORY:=Utilities
TITLE:=easymesh_ioctl
MAINTAINER:=fengchun.ye <fengchun.ye@siflower.com.cn>
DEPENDS:=+libc +libpthread +librt +libubus
endef

define Package/easymesh_ioctl/description
interface for easymesh driver.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./$(KO_FILE_NAME)/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/easymesh_ioctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/easymesh_ioctl
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/easymesh_ioctl $(1)/usr/bin
endef

$(eval $(call BuildPackage,easymesh_ioctl))
