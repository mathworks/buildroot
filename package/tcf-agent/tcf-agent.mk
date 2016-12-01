################################################################################
#
# Eclipse TCF Agent
#
################################################################################
TCF_AGENT_VERSION = 1.4_neon
TCF_AGENT_SITE_METHOD = git
TCF_AGENT_SITE = https://git.eclipse.org/r/tcf/org.eclipse.tcf.agent
TCF_AGENT_INSTALL_STAGING = YES
TCF_AGENT_LICENSE = EPLv1.0
TCF_AGENT_LICENSE_FILES = agent/about.html
# util-linux provides uuid lib
TCF_AGENT_DEPENDENCIES = openssl util-linux

TCF_AGENT_CONFIGURE_OPTS = $(TARGET_CONFIGURE_OPTS) 
TCF_AGENT_CONFIGURE_OPTS += OPSYS="GNU/Linux" MACHINE=$(BR2_ARCH) INSTALLROOT=$(STAGING_DIR)

define TCF_AGENT_BUILD_CMDS
    $(MAKE) $(TCF_AGENT_CONFIGURE_OPTS) -C $(@D)/agent all
endef

define TCF_AGENT_INSTALL_STAGING_CMDS
    $(MAKE) $(TCF_AGENT_CONFIGURE_OPTS) -C $(@D)/agent install
endef

define TCF_AGENT_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(STAGING_DIR)/usr/sbin/tcf-agent $(TARGET_DIR)/usr/sbin/tcf-agent
endef

define TCF_AGENT_INSTALL_INIT_SYSV
        $(INSTALL) -D -m 0755 package/tcf-agent/S99tcf_agent \
                $(TARGET_DIR)/etc/init.d/S99tcf_agent
endef


$(eval $(generic-package))
