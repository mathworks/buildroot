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

define TCF_AGENT_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/agent/agent $(TARGET_DIR)/usr/sbin/tcf-agent
endef

define TCF_AGENT_INSTALL_INIT_SYSV
        $(INSTALL) -D -m 0755 $(TCF_AGENT_PKGDIR)/S99tcf_agent \
                $(TARGET_DIR)/etc/init.d/S99tcf_agent
endef

$(eval $(cmake-package))
