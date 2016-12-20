################################################################################
#
# Recovery Image Generator
#
################################################################################

RECOVERY_IMAGE_INSTALL_TARGET = NO
RECOVERY_IMAGE_INSTALL_IMAGES = YES
RECOVERY_IMAGE_LICENSE = GPLv2

ifeq ($(BR2_PACKAGE_RECOVERY_IMAGE_USE_CATALOG),y)
RECOVERY_IMAGE_BUILD_SPEC = -c $(call qstrip,$(BR2_PACKAGE_RECOVERY_IMAGE_CATALOG))
else ifeq ($(BR2_PACKAGE_RECOVERY_IMAGE_USE_BRDPLT),y)
RECOVERY_IMAGE_BUILD_SPEC = -b $(call qstrip,$(BR2_PACKAGE_RECOVERY_IMAGE_BOARD)) -p $(call qstrip,$(BR2_PACKAGE_RECOVERY_IMAGE_PLATFORM))
endif

RECOVERY_IMAGE_FILE = $(call qstrip,$(BR2_PACKAGE_RECOVERY_IMAGE_FILE))
RECOVERY_IMAGE_BUILD_SCRIPT = board/mathworks/common/scripts/build.py

define RECOVERY_IMAGE_CONFIGURE_CMDS
	$(RECOVERY_IMAGE_BUILD_SCRIPT) $(RECOVERY_IMAGE_BUILD_SPEC) -u -o $(@D) --target clean
endef

define RECOVERY_IMAGE_BUILD_CMDS
	$(RECOVERY_IMAGE_BUILD_SCRIPT) $(RECOVERY_IMAGE_BUILD_SPEC) -u -o $(@D)
endef

define RECOVERY_IMAGE_INSTALL_IMAGES_CMDS
	cp -f $(@D)/images/$(RECOVERY_IMAGE_FILE) $(BINARIES_DIR)/
endef

$(eval $(generic-package))
