################################################################################
#
# libfoo
#
################################################################################

FRU_TOOLS_VERSION = 7d81a1e96babd94375034aa2a347ede3be45dc3f
FRU_TOOLS_SITE = $(call github,analogdevicesinc,fru_tools,$(FRU_TOOLS_VERSION))
FRU_TOOLS_LICENSE = GPL-2.0
FRU_TOOLS_LICENSE_FILES = license.txt

define FRU_TOOLS_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) ALL
endef

define FRU_TOOLS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/fru-dump $(TARGET_DIR)/usr/bin/fru-dump
endef

$(eval $(generic-package))
