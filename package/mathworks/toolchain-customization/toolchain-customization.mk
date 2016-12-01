# Some toolchains, such as Linaro, expect the libraries in
# {/usr,}/lib/<tuple>, but Buildroot copies them to
# {/usr,}/lib, so we need to create a symbolic link.
ifeq ($(BR2_PACKAGE_TOOLCHAIN_CUSTOMIZATION_NEEDS_PREFIX_SYMLINK),y)
define TOOLCHAIN_CUSTOMIZATION_PREFIX_SYMLINK
	ln -snf . $(TARGET_DIR)/lib/$(TOOLCHAIN_EXTERNAL_PREFIX)
	ln -snf . $(TARGET_DIR)/usr/lib/$(TOOLCHAIN_EXTERNAL_PREFIX)
endef
TOOLCHAIN_EXTERNAL_POST_INSTALL_STAGING_HOOKS += TOOLCHAIN_CUSTOMIZATION_PREFIX_SYMLINK
endif

$(eval $(generic-package))
