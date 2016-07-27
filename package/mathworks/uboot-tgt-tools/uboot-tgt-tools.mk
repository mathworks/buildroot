################################################################################
#
# uboot-tools
#
################################################################################

ifeq ($(BR2_PACKAGE_UBOOT_TGT_TOOLS_USECUSTOMGIT),y)
ifeq ($(BR2_TARGET_UBOOT_CUSTOM_GIT),y)
UBOOT_TGT_TOOLS_VERSION     = $(call qstrip,$(BR2_TARGET_UBOOT_VERSION))
UBOOT_TGT_TOOLS_BOARD_NAME = $(call qstrip,$(BR2_TARGET_UBOOT_BOARDNAME))
UBOOT_TGT_TOOLS_SITE        = $(call qstrip,$(BR2_TARGET_UBOOT_CUSTOM_REPO_URL))
else
UBOOT_TGT_TOOLS_VERSION     = $(call qstrip,$(BR2_PACKAGE_UBOOT_ALTERA_VERSION))
UBOOT_TGT_TOOLS_BOARD_NAME = $(call qstrip,$(BR2_PACKAGE_UBOOT_ALTERA_BOARDNAME))
UBOOT_TGT_TOOLS_SITE        = $(call qstrip,$(BR2_PACKAGE_UBOOT_ALTERA_CUSTOM_REPO_URL))
endif
UBOOT_TGT_TOOLS_SITE_METHOD = git
else
UBOOT_TGT_TOOLS_VERSION = 2015.04
UBOOT_TGT_TOOLS_SOURCE = u-boot-$(UBOOT_TGT_TOOLS_VERSION).tar.bz2
UBOOT_TGT_TOOLS_SITE = ftp://ftp.denx.de/pub/u-boot
endif
UBOOT_TGT_TOOLS_LICENSE = GPLv2+
UBOOT_TGT_TOOLS_LICENSE_FILES = Licenses/gpl-2.0.txt

# Helper function to fill the U-Boot config.h file.
# Argument 1: option name
# Argument 2: option value
# If the option value is empty, this function does nothing.
define insert_define
$(if $(call qstrip,$(2)),
	@echo "#ifdef $(strip $(1))" >> $(@D)/include/config.h
	@echo "#undef $(strip $(1))" >> $(@D)/include/config.h
	@echo "#endif" >> $(@D)/include/config.h
	@echo '#define $(strip $(1)) $(call qstrip,$(2))' >> $(@D)/include/config.h)
endef

UBOOT_TGT_TOOLS_ARCH = $(KERNEL_ARCH)

UBOOT_TGT_TOOLS_MAKE_OPTS += \
	CROSS_COMPILE=$(TARGET_CROSS) \
	ARCH=$(UBOOT_TGT_TOOLS_ARCH)

define UBOOT_TGT_TOOLS_CONFIGURE_CMDS
	$(TARGET_MAKE_ENV)  \
		$(MAKE) -C $(@D) $(UBOOT_TGT_TOOLS_MAKE_OPTS)   \
		$(UBOOT_TGT_TOOLS_BOARD_NAME)_config
	@echo >> $(@D)/include/config.h
	@echo "/* Add a wrapper around the values Buildroot sets. */" >> $(@D)/include/config.h
	@echo "#ifndef __BR2_ADDED_CONFIG_H" >> $(@D)/include/config.h
	@echo "#define __BR2_ADDED_CONFIG_H" >> $(@D)/include/config.h
	$(call insert_define,DATE,$(DATE))
	$(call insert_define,CONFIG_LOAD_SCRIPTS,1)
	$(call insert_define,CONFIG_IPADDR,$(BR2_TARGET_UBOOT_IPADDR))
	$(call insert_define,CONFIG_GATEWAYIP,$(BR2_TARGET_UBOOT_GATEWAY))
	$(call insert_define,CONFIG_NETMASK,$(BR2_TARGET_UBOOT_NETMASK))
	$(call insert_define,CONFIG_SERVERIP,$(BR2_TARGET_UBOOT_SERVERIP))
	$(call insert_define,CONFIG_ETHADDR,$(BR2_TARGET_UBOOT_ETHADDR))
	$(call insert_define,CONFIG_ETH1ADDR,$(BR2_TARGET_UBOOT_ETH1ADDR))
	@echo "#endif /* __BR2_ADDED_CONFIG_H */" >> $(@D)/include/config.h
endef


define UBOOT_TGT_TOOLS_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) 	\
    $(UBOOT_TGT_TOOLS_MAKE_OPTS) \
    tools env
endef

ifeq ($(BR2_PACKAGE_UBOOT_TGT_TOOLS_MKIMAGE),y)
define UBOOT_TGT_TOOLS_INSTALL_MKIMAGE
	$(INSTALL) -m 0755 -D $(@D)/tools/mkimage $(TARGET_DIR)/usr/bin/mkimage
endef
endif # BR2_PACKAGE_UBOOT_TGT_TOOLS_MKIMAGE

ifeq ($(BR2_PACKAGE_UBOOT_TGT_TOOLS_MKENVIMAGE),y)
define UBOOT_TGT_TOOLS_INSTALL_MKENVIMAGE
	$(INSTALL) -m 0755 -D $(@D)/tools/mkenvimage $(TARGET_DIR)/usr/bin/mkenvimage
endef
endif

ifeq ($(BR2_PACKAGE_UBOOT_TGT_TOOLS_FWPRINTENV),y)
define UBOOT_TGT_TOOLS_INSTALL_FWPRINTENV
	$(INSTALL) -m 0755 -D $(@D)/tools/env/fw_printenv $(TARGET_DIR)/usr/sbin/fw_printenv
	ln -sf fw_printenv $(TARGET_DIR)/usr/sbin/fw_setenv
endef
endif

define UBOOT_TGT_TOOLS_INSTALL_TARGET_CMDS
	$(UBOOT_TGT_TOOLS_INSTALL_MKIMAGE)
	$(UBOOT_TGT_TOOLS_INSTALL_MKENVIMAGE)
	$(UBOOT_TGT_TOOLS_INSTALL_FWPRINTENV)
endef

define HOST_UBOOT_TGT_TOOLS_BUILD_CMDS
	$(MAKE1) -C $(@D) 			\
		HOSTCC="$(HOSTCC)"		\
		HOSTCFLAGS="$(HOST_CFLAGS)"	\
		HOSTLDFLAGS="$(HOST_LDFLAGS)"	\
		tools env
endef

define HOST_UBOOT_TGT_TOOLS_INSTALL_CMDS
	$(INSTALL) -m 0755 -D $(@D)/tools/mkimage $(HOST_DIR)/usr/bin/mkimage
	$(INSTALL) -m 0755 -D $(@D)/tools/mkenvimage $(HOST_DIR)/usr/bin/mkenvimage
endef

$(eval $(generic-package))
$(eval $(host-generic-package))

# Convenience variables for other mk files that make use of mkimage

MKIMAGE = $(HOST_DIR)/usr/bin/mkimage

# mkimage supports arm blackfin m68k microblaze mips mips64 nios2 powerpc ppc sh sparc sparc64 x86
# KERNEL_ARCH can be arm64 arc arm blackfin m68k microblaze mips nios2 powerpc sh sparc i386 x86_64 xtensa
# For arm64, arc, xtensa we'll just keep KERNEL_ARCH
# For mips64, we'll just keep mips
# For i386 and x86_64, we need to convert
ifeq ($(KERNEL_ARCH),x86_64)
MKIMAGE_ARCH = x86
else ifeq ($(KERNEL_ARCH),i386)
MKIMAGE_ARCH = x86
else
MKIMAGE_ARCH = $(KERNEL_ARCH)
endif
