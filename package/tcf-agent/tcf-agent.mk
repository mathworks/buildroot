################################################################################
#
# Eclipse TCF Agent
#
################################################################################
TCF_AGENT_VERSION = 1.4_neon
TCF_AGENT_SITE_METHOD = git
TCF_AGENT_SITE = https://git.eclipse.org/r/tcf/org.eclipse.tcf.agent
TCF_AGENT_LICENSE = EPLv1.0
TCF_AGENT_LICENSE_FILES = agent/about.html
TCF_AGENT_SUBDIR = agent
TCF_AGENT_INSTALL_STAGING = YES

TCF_AGENT_CONF_OPTS=-DTCF_MACHINE:STRING=$(BR2_PACKAGE_TCF_AGENT_MACHINE)

ifneq ($(BR2_PACKAGE_TCF_AGENT_USESSL),y)
TCF_AGENT_CONFIGURE_OPTS += NO_SSL=1
else
TCF_AGENT_DEPENDENCIES += openssl
endif

ifneq ($(BR2_PACKAGE_TCF_AGENT_USEUUID),y)
TCF_AGENT_CONFIGURE_OPTS += NO_UUID=1
else
# util-linux provides uuid lib
TCF_AGENT_DEPENDENCIES += util-linux
endif

define TCF_AGENT_INSTALL_STAGING_CMDS
    $(INSTALL) -D -m 0755 $(@D)/$(TCF_AGENT_SUBDIR)/libtcf-agent.so* $(STAGING_DIR)/usr/lib
    $(INSTALL) -D -m 0644 $(@D)/$(TCF_AGENT_SUBDIR)/tcf/config.h $(STAGING_DIR)/usr/include/tcf/config.h
	mkdir -p $(STAGING_DIR)/usr/include/tcf/framework
    $(INSTALL) -D -m 0644 $(@D)/$(TCF_AGENT_SUBDIR)/tcf/framework/*.h $(STAGING_DIR)/usr/include/tcf/framework
	mkdir -p $(STAGING_DIR)/usr/include/tcf/services
    $(INSTALL) -D -m 0644 $(@D)/$(TCF_AGENT_SUBDIR)/tcf/services/*.h $(STAGING_DIR)/usr/include/tcf/services
endef

define TCF_AGENT_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/$(TCF_AGENT_SUBDIR)/agent $(TARGET_DIR)/usr/sbin/tcf-agent
    $(INSTALL) -D -m 0755 $(@D)/$(TCF_AGENT_SUBDIR)/libtcf-agent.so* $(TARGET_DIR)/usr/lib
endef

define TCF_AGENT_INSTALL_INIT_SYSV
        $(INSTALL) -D -m 0755 $(TCF_AGENT_PKGDIR)/S99tcf_agent \
                $(TARGET_DIR)/etc/init.d/S99tcf_agent
endef

$(eval $(cmake-package))
