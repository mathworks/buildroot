################################################################################
#
# Altera U-Boot Modifications
#
################################################################################

UBOOT_ALTERA_EDS_DIR = $(call qstrip,$(BR2_PACKAGE_UBOOT_ALTERA_EDS_INSTALLATION))

UBOOT_ALTERA_INSTALL_TARGET = NO
UBOOT_ALTERA_INSTALL_IMAGES = NO
UBOOT_ALTERA_INSTALL_STAGING = YES

UBOOT_ALTERA_STAGING_TARGET = usr/share/uboot-altera

UBOOT_ALTERA_BSP_TOOLS = $(UBOOT_ALTERA_EDS_DIR)/host_tools/altera/preloadergen
UBOOT_ALTERA_QUARTUS_HANDOFF_DIR = $(call qstrip,$(BR2_PACKAGE_UBOOT_ALTERA_QUARTUS_HANDOFF_DIR))
UBOOT_ALTERA_BSP_OPTIONS = $(call qstrip,$(BR2_PACKAGE_UBOOT_ALTERA_BSP_OPTIONS))

UBOOT_ALTERA_UBOOT_DIR = uboot-$(call qstrip,$(BR2_TARGET_UBOOT_VERSION))
UBOOT_ALTERA_UBOOT_BUILD_DIR = $(BUILD_DIR)/$(UBOOT_ALTERA_UBOOT_DIR)
UBOOT_ALTERA_STAGING_HANDOFF = $(STAGING_DIR)/$(UBOOT_ALTERA_STAGING_TARGET)/handoff
UBOOT_ALTERA_STAGING_GENERATED = $(STAGING_DIR)/$(UBOOT_ALTERA_STAGING_TARGET)/generated

define UBOOT_ALTERA_UPDATE_UBOOT
    @echo ">>> Copying handoff files from $(UBOOT_ALTERA_STAGING_HANDOFF)"
    cp -R -f $(UBOOT_ALTERA_STAGING_HANDOFF)/* $(UBOOT_ALTERA_UBOOT_BUILD_DIR)/board/altera/socfpga/sdram

    @echo ">>> Copying generated files from $(UBOOT_ALTERA_STAGING_GENERATED)"
    cp -R -f $(UBOOT_ALTERA_STAGING_GENERATED)/* $(UBOOT_ALTERA_UBOOT_BUILD_DIR)/board/altera/socfpga
endef

ifeq ($(BR2_PACKAGE_UBOOT_ALTERA),y)
UBOOT_PRE_CONFIGURE_HOOKS += UBOOT_ALTERA_UPDATE_UBOOT
UBOOT_DEPENDENCIES += uboot-altera
endif

# Generate the BSP based on the handoff files
define UBOOT_ALTERA_GENERATE_BSP
     $(UBOOT_ALTERA_BSP_TOOLS)/bsp-create-settings \
        --type spl --bsp-dir $(@D)/bsp \
        --settings $(@D)/bsp/settings.bsp \
        --preloader-settings-dir $(UBOOT_ALTERA_QUARTUS_HANDOFF_DIR) \
        $(UBOOT_ALTERA_BSP_OPTIONS)
endef

ifeq ($(BR2_PACKAGE_UBOOT_ALTERA_GENERATE_BSP),y)
# We are building the BSP
define UBOOT_ALTERA_BUILD_CMDS
    $(UBOOT_ALTERA_GENERATE_BSP)
endef
UBOOT_ALTERA_QUARTUS_HANDOFF_SRC = $(UBOOT_ALTERA_QUARTUS_HANDOFF_DIR)
UBOOT_ALTERA_QUARTUS_BSP_SRC = $(@D)/bsp/generated
else
# We are using the pre-built BSP
UBOOT_ALTERA_QUARTUS_HANDOFF_SRC = $(UBOOT_ALTERA_QUARTUS_HANDOFF_DIR)/handoff
UBOOT_ALTERA_QUARTUS_BSP_SRC = $(UBOOT_ALTERA_QUARTUS_HANDOFF_DIR)/generated
endif

define UBOOT_ALTERA_INSTALL_STAGING_CMDS
    mkdir -p $(UBOOT_ALTERA_STAGING_GENERATED)
	cp -Rf $(UBOOT_ALTERA_QUARTUS_BSP_SRC)/* $(UBOOT_ALTERA_STAGING_GENERATED)/
    mkdir -p $(UBOOT_ALTERA_STAGING_HANDOFF)
    cp -Rf $(UBOOT_ALTERA_QUARTUS_HANDOFF_SRC)/* $(UBOOT_ALTERA_STAGING_HANDOFF)/
endef

$(eval $(generic-package))
