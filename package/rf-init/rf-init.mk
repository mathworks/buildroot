
################################################################################
#
# rf-init application 
#
################################################################################
RFINIT_VERSION = 1.0.0
RFINIT_SITE = package/rf-init
#RFINIT_SITE = 
RFINIT_SITE_METHOD = local
#RFINIT_INSTALL_TARGET=YES

define RFINIT_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define RFINIT_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/embeddedinn  $(TARGET_DIR)/usr/bin
endef
$(eval $(generic-package))



