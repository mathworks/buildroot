#ifndef __ZYNQ_MW_AXISTREAM_IIO_COMMON__
#define __ZYNQ_MW_AXISTREAM_IIO_COMMON__

#include "zynq-mw-axistream-common.h"

/* Select ADI or Xilinx DMA */
#if defined(MW_DMA_SELECT_ADI)
#include "adi-mw-axistream-iio-common.dtsi"
#else
#include "xilinx-mw-axistream-iio-common.dtsi"
#endif

#endif /* __ZYNQ_MW_AXISTREAM_IIO_COMMON__ */