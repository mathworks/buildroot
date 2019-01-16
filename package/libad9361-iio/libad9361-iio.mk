################################################################################
#
# libad9361-iio
#
################################################################################

LIBAD9361_IIO_VERSION = 0.2
LIBAD9361_IIO_SITE = $(call github,analogdevicesinc,libad9361-iio,v$(LIBAD9361_IIO_VERSION))

LIBAD9361_IIO_INSTALL_STAGING = YES
LIBAD9361_IIO_LICENSE = LGPL-2.1+
LIBAD9361_IIO_LICENSE_FILES = LICENSE

ifeq ($(BR2_PACKAGE_LIBIIO),y)
LIBAD9361_IIO_DEPENDENCIES += libiio
endif

$(eval $(cmake-package))
