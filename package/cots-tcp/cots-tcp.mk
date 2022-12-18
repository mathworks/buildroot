
################################################################################
#
# cots tcp
#
################################################################################
COTS-TCP_VERSION:= 1.0.0
COTS-TCP_SITE = package/cots-tcp
COTS-TCP_SITE_METHOD:=local
COTS-TCP_INSTALL_TARGET:=YES

define COTS-TCP_BUILD_CMDS
$(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) all
endef

define COTS-TCP_INSTALL_TARGET_CMDS
$(INSTALL) -D -m 0755 $(@D)/cots-tcp $(TARGET_DIR)/bin/cots-tcp
endef

#define COTS-TCP_PERMISSIONS
#/mnt/cots-tcp f 4755 0 0 - - - - -
#endef

$(eval $(generic-package))


