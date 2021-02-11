################################################################################
#
# lttng-modules-2.11.6
#
################################################################################

LTTNG_MODULES_2_11_6_VERSION = 2.11.6
LTTNG_MODULES_2_11_6_SITE = http://lttng.org/files/lttng-modules
LTTNG_MODULES_2_11_6_SOURCE = lttng-modules-$(LTTNG_MODULES_2_11_6_VERSION).tar.bz2
LTTNG_MODULES_2_11_6_LICENSE = LGPL-2.1/GPL-2.0 (kernel modules), MIT (lib/bitfield.h, lib/prio_heap/*)
LTTNG_MODULES_2_11_6_LICENSE_FILES = LICENSES/LGPL-2.1 LICENSES/GPL-2.0 LICENSES/MIT LICENSE
LTTNG_MODULES_2_11_6_MODULE_MAKE_OPTS = CONFIG_LTTNG=m CONFIG_LTTNG_CLOCK_PLUGIN_TEST=m

$(eval $(kernel-module))
$(eval $(generic-package))
