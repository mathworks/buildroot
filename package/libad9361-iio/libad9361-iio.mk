################################################################################
#
# libad9361-iio
#
################################################################################

LIBAD9361_IIO_VERSION = 4c04680a34730efd7ec3f46daacdc4cf9a2327d7
LIBAD9361_IIO_SOURCE  = libad9361-iio-$(LIBAD9361_IIO_VERSION).tar.gz
LIBAD9361_IIO_SITE = https://github.com/analogdevicesinc/libad9361-iio/archive/$(LIBAD9361_IIO_VERSION)/$(LIBAD9361_IIO_SOURCE)
LIBAD9361_IIO_INSTALL_STAGING = YES
LIBAD9361_IIO_LICENSE = LGPL-2.1+
LIBAD9361_IIO_LICENSE_FILES = LICENSE

ifeq ($(BR2_PACKAGE_LIBIIO),y)
LIBAD9361_IIO_DEPENDENCIES += libiio
endif

$(eval $(cmake-package))
