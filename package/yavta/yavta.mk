################################################################################
#
# yavta
#
################################################################################

YAVTA_VERSION = 65f740aa1758531fd810339bc1b7d1d33666e28a
YAVTA_SITE = git://git.ideasonboard.org/yavta.git
YAVTA_LICENSE = GPL-2.0+
YAVTA_LICENSE_FILES = COPYING.GPL

define YAVTA_BUILD_CMDS
	cp -a $(@D)/include/linux/videodev2.h  $(STAGING_DIR)/usr/include/linux/videodev2.h
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define YAVTA_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 -D $(@D)/yavta $(TARGET_DIR)/usr/bin/yavta
endef

$(eval $(generic-package))
