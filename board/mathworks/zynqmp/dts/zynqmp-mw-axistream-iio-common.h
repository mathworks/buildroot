#ifndef __ZYNQMP_MW_AXISTREAM_IIO_COMMON__
#define __ZYNQMP_MW_AXISTREAM_IIO_COMMON__

/* PL-to-PS IRQ starts at 0x59 for ZynqMP */
#if defined(MW_DMA_MM2S_IRQ_SELECT)
#define MW_DMA_MM2S_IRQ  MW_DMA_MM2S_IRQ_SELECT
#else
#define MW_DMA_MM2S_IRQ  0x59
#endif
#if defined(MW_DMA_S2MM_IRQ_SELECT)
#define MW_DMA_S2MM_IRQ  MW_DMA_S2MM_IRQ_SELECT
#else
#define MW_DMA_S2MM_IRQ  0x60
#endif

/* Select ADI or Xilinx DMA */
#if defined(MW_DMA_SELECT_ADI)
#include "adi-mw-axistream-iio-common.dtsi"
#else
#include "xilinx-mw-axistream-iio-common.dtsi"
#endif

/* Use 128-bit HP AXI */
#define MW_AXI_MM_DATAWIDTH_SELECT 128

#endif /* __ZYNQMP_MW_AXISTREAM_IIO_COMMON__ */