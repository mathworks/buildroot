#ifndef __SOCFPGA_MW_AXISTREAM_IIO_COMMON__ 
#define __SOCFPGA_MW_AXISTREAM_IIO_COMMON__ 
   
/* FPGA-to-HPS IRQ no. are device-specific, define MW_DMA_*_IRQ_SELECT in board base.dtsi */ 
#define MW_DMA_MM2S_IRQ  MW_DMA_MM2S_IRQ_SELECT 
#define MW_DMA_S2MM_IRQ  MW_DMA_S2MM_IRQ_SELECT 
       
/* Use the ADI DMA */ 
#include "adi-mw-axistream-iio-common.dtsi" 
#endif /* __SOCFPGA_MW_AXISTREAM_IIO_COMMON__ */ 

