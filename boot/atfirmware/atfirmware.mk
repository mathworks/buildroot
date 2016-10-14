################################################################################
#
# ARM Trusted Firmware
#
################################################################################

ATFIRMWARE_VERSION = $(call qstrip,$(BR2_TARGET_ATFIRMWARE_VERSION))

#package dependencies
ATFIRMWARE_DEPENDENCIES += uboot

ifeq ($(ATFIRMWARE_VERSION),custom)
# Handle custom ATF tarballs as specified by the configuration
ATFIRMWARE_TARBALL = $(call qstrip,$(BR2_TARGET_ATFIRMWARE_CUSTOM_TARBALL_LOCATION))
ATFIRMWARE_SITE = $(patsubst %/,%,$(dir $(ATFIRMWARE_TARBALL)))
ATFIRMWARE_SOURCE = $(notdir $(ATFIRMWARE_TARBALL))
BR_NO_CHECK_HASH_FOR += $(ATFIRMWARE_SOURCE)
else ifeq ($(BR2_TARGET_ATFIRMWARE_CUSTOM_GIT),y)
ATFIRMWARE_SITE = $(call qstrip,$(BR2_TARGET_ATFIRMWARE_CUSTOM_GIT_REPO_URL))
ATFIRMWARE_SITE_METHOD = git
endif

ifneq ($(call qstrip,$(BR2_TARGET_ATFIRMWARE_CUSTOM_PATCH_DIR)),)
define ATFIRMWARE_APPLY_CUSTOM_PATCHES
	$(APPLY_PATCHES) $(@D) \
		$(BR2_TARGET_ATFIRMWARE_CUSTOM_PATCH_DIR) \*.patch
endef

ATFIRMWARE_POST_PATCH_HOOKS += ATFIRMWARE_APPLY_CUSTOM_PATCHES
endif

ATFIRMWARE_INSTALL_IMAGES = YES

# Automatically find the U-Boot binary
ifeq ($(call qstrip,$(BR2_TARGET_UBOOT_FORMAT_CUSTOM_NAME)),)
ATFIRMWARE_PAYLOAD_PATH = $(BINARIES_DIR)/"u-boot.bin"
else
ATFIRMWARE_PAYLOAD_PATH = $(BINARIES_DIR)/$(call qstrip,$(BR2_TARGET_UBOOT_FORMAT_CUSTOM_NAME))
endif

ATFIRMWARE_PLATFORM = $(call qstrip,$(BR2_TARGET_ATFIRMWARE_PLATFORM))

#if juno target is selected, then lets configured the expected SCP firmware binary
ifeq ($(ATFIRMWARE_PLATFORM),juno)
ATFIRMWARE_SCP_FIRMWARE="SCP_BL2=$(BINARIES_DIR)/scp-fw.bin"
#configure the dependencie of scpfirmware package
ATFIRMWARE_DEPENDENCIES += scpfirmware
endif

ATFIRMWARE_MAKE_OPTS += \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	BL33=$(call qstrip,$(ATFIRMWARE_PAYLOAD_PATH)) \
	$(call qstrip,$(BR2_TARGET_ATFIRMWARE_ADDITIONAL_VARIABLES)) \
	$(ATFIRMWARE_SCP_FIRMWARE) \
	PLAT=$(ATFIRMWARE_PLATFORM) \
	all fip

define ATFIRMWARE_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) \
	$(MAKE) -C $(@D) $(ATFIRMWARE_MAKE_OPTS) \
	$(ATFIRMWARE_MAKE_TARGET)
endef

define ATFIRMWARE_INSTALL_IMAGES_CMDS
	cp -dpf $(@D)/build/$(ATFIRMWARE_PLATFORM)/release/*.bin $(BINARIES_DIR)/ ;
endef

# Configuration ckeck
ifeq ($(BR2_TARGET_ATFIRMWARE)$(BR_BUILDING),yy)

ifeq ($(ATFIRMWARE_VERSION),custom)
ifeq ($(call qstrip,$(BR2_TARGET_ATFIRMWARE_CUSTOM_TARBALL_LOCATION))),)
$(error No tarball location specified. Please check BR2_TARGET_ATFIRMWARE_CUSTOM_TARBALL_LOCATION))
endif
endif

ifeq ($(BR2_TARGET_ATFIRMWARE_CUSTOM_GIT),y)
ifeq ($(call qstrip,$(BR2_TARGET_ATFIRMWARE_CUSTOM_GIT_REPO_URL)),)
$(error No repository specified. Please check BR2_TARGET_ATFIRMWARE_CUSTOM_GIT_REPO_URL)
endif
endif

endif

$(eval $(generic-package))
