################################################################################
#
# uboot-tools
#
################################################################################

ifeq ($(BR2_PACKAGE_UBOOT_TOOLS_USECUSTOMGIT),y)
UBOOT_TOOLS_VERSION     = $(call qstrip,$(BR2_TARGET_UBOOT_VERSION))
UBOOT_TOOLS_BOARD_NAME = $(call qstrip,$(BR2_TARGET_UBOOT_BOARDNAME))
UBOOT_TOOLS_SITE        = $(call qstrip,$(BR2_TARGET_UBOOT_CUSTOM_GIT_REPO_URL))
UBOOT_TOOLS_SITE_METHOD = git
UBOOT_TOOLS_CONFIGURE_OPTS = $(TARGET_CONFIGURE_OPTS)
else
UBOOT_TOOLS_VERSION = 2013.07
UBOOT_TOOLS_SOURCE  = u-boot-$(UBOOT_TOOLS_VERSION).tar.bz2
UBOOT_TOOLS_SITE    = ftp://ftp.denx.de/pub/u-boot
endif

UBOOT_TOOLS_MAKE_OPTS += \
	HOSTCC="$(TARGET_CC)"		\
	HOSTCFLAGS="$(TARGET_CFLAGS)"	\
	HOSTLDFLAGS="$(TARGET_LDFLAGS)"	\
	HOSTSTRIP=true


ifeq ($(BR2_PACKAGE_UBOOT_TOOLS_USECUSTOMGIT),y)
UBOOT_TOOLS_ARCH=$(KERNEL_ARCH)
UBOOT_TOOLS_MAKE_OPTS += \
	CROSS_COMPILE="$(CCACHE) $(TARGET_CROSS)" \
	ARCH=$(UBOOT_TOOLS_ARCH)
endif

UBOOT_TOOLS_LICENSE = GPLv2+
UBOOT_TOOLS_LICENSE_FILES = COPYING

ifeq ($(BR2_PACKAGE_UBOOT_TOOLS_USEBOARDCONFIG),y)
define UBOOT_TOOLS_CONFIGURE_CMDS
    $(UBOOT_TOOLS_CONFIGURE_OPTS)   \
        $(MAKE) -C $(@D) 			\
        $(UBOOT_TOOLS_MAKE_OPTS)    \
		$(UBOOT_TOOLS_BOARD_NAME)_config	
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
else
define UBOOT_TOOLS_APPLY_CONFIG_PATCH
    support/scripts/apply-patches.sh $(@D) package/uboot-tools uboot-tools-01-drop-configh-from-tools.patch.conditional
endef    
UBOOT_TOOLS_PRE_PATCH_HOOKS += \
	UBOOT_TOOLS_APPLY_CONFIG_PATCH
endif


define UBOOT_TOOLS_BUILD_CMDS
	$(UBOOT_TOOLS_CONFIGURE_OPTS)   \
	    $(MAKE) -C $(@D) 		    \
		$(UBOOT_TOOLS_MAKE_OPTS)    \
		tools env
endef

ifeq ($(BR2_PACKAGE_UBOOT_TOOLS_MKIMAGE),y)
define UBOOT_TOOLS_INSTALL_MKIMAGE
	$(INSTALL) -m 0755 -D $(@D)/tools/mkimage $(TARGET_DIR)/usr/bin/mkimage
endef
endif

ifeq ($(BR2_PACKAGE_UBOOT_TOOLS_MKENVIMAGE),y)
define UBOOT_TOOLS_INSTALL_MKENVIMAGE
	$(INSTALL) -m 0755 -D $(@D)/tools/mkenvimage $(TARGET_DIR)/usr/bin/mkenvimage
endef
endif

ifeq ($(BR2_PACKAGE_UBOOT_TOOLS_FWPRINTENV),y)
define UBOOT_TOOLS_INSTALL_FWPRINTENV
	$(INSTALL) -m 0755 -D $(@D)/tools/env/fw_printenv $(TARGET_DIR)/usr/sbin/fw_printenv
	ln -sf fw_printenv $(TARGET_DIR)/usr/sbin/fw_setenv
endef
endif

define UBOOT_TOOLS_INSTALL_TARGET_CMDS
	$(UBOOT_TOOLS_INSTALL_MKIMAGE)
	$(UBOOT_TOOLS_INSTALL_MKENVIMAGE)
	$(UBOOT_TOOLS_INSTALL_FWPRINTENV)
endef

define UBOOT_TOOLS_UNINSTALL_TARGET_CMDS
	rm -f $(addprefix $(TARGET_DIR)/,\
		usr/bin/mkimage usr/sbin/fw_printenv usr/sbin/fw_setenv)
endef

define HOST_UBOOT_TOOLS_BUILD_CMDS
	$(MAKE1) -C $(@D) 			\
		HOSTCC="$(HOSTCC)"		\
		HOSTCFLAGS="$(HOST_CFLAGS)"	\
		HOSTLDFLAGS="$(HOST_LDFLAGS)"	\
		tools
endef

define HOST_UBOOT_TOOLS_INSTALL_CMDS
	$(INSTALL) -m 0755 -D $(@D)/tools/mkimage $(HOST_DIR)/usr/bin/mkimage
	$(INSTALL) -m 0755 -D $(@D)/tools/mkenvimage $(HOST_DIR)/usr/bin/mkenvimage
endef

$(eval $(generic-package))
$(eval $(host-generic-package))
