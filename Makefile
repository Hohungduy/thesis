#
# Copyright (c) 2020 Duyho
# This is free software, lisenced under the GNU General Public Lisence v2.
#
#  $Id$

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=thesis
PKG_VERSION:=1.10
PKG_RELEASE:=1
 
# PKG_SOURCE_URL:=https://github.com/Hohungduy/Testcrypto.git 
# PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
# PKG_SOURCE_PROTO:=git
# PKG_MIRROR_HASH:=skip
#PKG_HASH:=22f8640fa5afdb9ef3353f9179a497fccde4502c6cfb2a4c035872a94917d6d7
# PKG_HASH:=skip
# PKG_SOURCE_VERSION=$(PKG_VERSION)
# PKG_LICENSE:=GPL-3.0
# PKG_LICENSE_FILES:=COPYING

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)
MODULE_FILES=$(PKG_BUILD_DIR)/*.$(LINUX_KMOD_SUFFIX)

INC_DIR = ./$(SUBDIRS)/../include
# MODULE_HEADER:=$(PKG_NAME).h

include $(INCLUDE_DIR)/package.mk

define KernelPackage/thesis
	SUBMENU:=Cryptographic API modules
	DEFAULT:=y if ALL
	TITLE:=Driver for test cryptographic acceleration
	URL:=http://google.vn
	VERSION:=$(LINUX_VERSION)+$(PKG_VERSION)-$(BOARD)-$(PKG_RELEASE)
	DEPENDS:=+kmod-crypto-authenc +kmod-crypto-hash +kmod-crypto-md5 +kmod-crypto-hmac +kmod-crypto-sha256 +kmod-crypto-sha512
	FILES:=$(MODULE_FILES)
	AUTOLOAD:=$(call AutoLoad,50,thesis)
	$(call AddDepends/crypto)
endef

define KernelPackage/thesis/description
	This is prototype for testing hardware accelerator engine driver for FPGA cards (PCIe)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	pwd
	$(CP) -r ./numato-driver/xdma_driver_wait/* $(PKG_BUILD_DIR)/
	$(CP) -r ./numato-driver/xdma_driver_wait/include/* $(PKG_BUILD_DIR)/xdma
endef

EXTRA_KCONFIG:= \
	CONFIG_TESTCRYPTO=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)/xdma" \
	TARGET="$(HAL_TARGET)" \
	TOOLPREFIX="$(KERNEL_CROSS)" \
	TOOLPATH="$(KERNEL_CROSS)" \
	KERNELPATH="$(LINUX_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Configure
endef

# define Build/Compile
# 	  $(MAKE) -C $(PKG_BUILD_DIR) \
# 		$(KERNEL_MAKE_FLAGS) \
# 		KERNEL_DIR="$(LINUX_DIR)" all
# endef

# define Build/Compile
#     $(MAKE) -C "$(LINUX_DIR)" \
#         $(MAKE_OPTS) \
#         modules
# endef

define Build/Compile
	pwd
	$(MAKE) -C "$(LINUX_DIR)" \
	$(MAKE_OPTS) \
	modules
endef

define Build/InstallDev
	# $(INSTALL_DIR) $(STAGING_DIR)/usr/include/crypto
	# $(CP) $(PKG_BUILD_DIR)/testcrypto.h $(STAGING_DIR)/usr/include/crypto/
endef

$(eval $(call KernelPackage,thesis))


