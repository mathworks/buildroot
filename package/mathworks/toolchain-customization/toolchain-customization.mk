################################################################################
#
# toolchain-customization
#
################################################################################

TOOLCHAIN_CUSTOMIZATION_ARCH_LD = $(call qstrip,$(BR2_PACKAGE_TOOLCHAIN_CUSTOMIZATION_ARCH_LD))

# Some toolchains, such as Linaro, expect the libraries in
# {/usr,}/lib/<tuple>, but Buildroot copies them to
# {/usr,}/lib, so we need to create a symbolic link.
ifeq ($(BR2_PACKAGE_TOOLCHAIN_CUSTOMIZATION_NEEDS_PREFIX_SYMLINK),y)
define TOOLCHAIN_CUSTOMIZATION_PREFIX_SYMLINK
	ln -snf . $(TARGET_DIR)/lib/$(TOOLCHAIN_EXTERNAL_PREFIX)
	ln -snf . $(TARGET_DIR)/usr/lib/$(TOOLCHAIN_EXTERNAL_PREFIX)
endef
TOOLCHAIN_EXTERNAL_CUSTOM_POST_INSTALL_STAGING_HOOKS += TOOLCHAIN_CUSTOMIZATION_PREFIX_SYMLINK
endif

# Some older linaro toolchains need to have the ld-*.so.* file
# copied over manually, as they are incompatible with the buildroot logic
copy_custom_ld = \
	ARCH_SYSROOT_DIR="$(call toolchain_find_sysroot,$(TOOLCHAIN_EXTERNAL_CC) $(TOOLCHAIN_EXTERNAL_CFLAGS))"; \
	CUSTOM_LD=$${ARCH_SYSROOT_DIR}/lib/$(TOOLCHAIN_CUSTOMIZATION_ARCH_LD); \
	STAGING_LD=$(STAGING_DIR)/lib/$(TOOLCHAIN_CUSTOMIZATION_ARCH_LD); \
	if [ ! -e "$${CUSTOM_LD}" ]; then \
		echo "Could not find $${CUSTOM_LD}, check BR2_PACKAGE_TOOLCHAIN_CUSTOMIZATION_ARCH_LD"; \
		exit 1; \
	fi; \
	cp -a $${CUSTOM_LD} $(STAGING_DIR)/lib/;

ifeq ($(BR2_PACKAGE_TOOLCHAIN_CUSTOMIZATION_COPY_ARCH_LD),y)

ifeq ($(TOOLCHAIN_CUSTOMIZATION_ARCH_LD),)
$(error BR2_PACKAGE_TOOLCHAIN_CUSTOMIZATION_ARCH_LD cannot be empty)
endif

define TOOLCHAIN_CUSTOMIZATION_COPY_LD
	$(Q)$(call copy_custom_ld)
endef

TOOLCHAIN_EXTERNAL_CUSTOM_POST_BUILD_HOOKS += TOOLCHAIN_CUSTOMIZATION_COPY_LD
endif

$(eval $(generic-package))
