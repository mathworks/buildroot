#ifndef __ZYNQ_MW_AXISTREAM_COMMON__
#define __ZYNQ_MW_AXISTREAM_COMMON__

/* PL-to-PS IRQ starts at 0x1d for Zynq-7000 */
#if defined(MW_DMA_MM2S_IRQ_SELECT)
#define MW_DMA_MM2S_IRQ  MW_DMA_MM2S_IRQ_SELECT
#else
#define MW_DMA_MM2S_IRQ  0x1d
#endif
#if defined(MW_DMA_S2MM_IRQ_SELECT)
#define MW_DMA_S2MM_IRQ  MW_DMA_S2MM_IRQ_SELECT
#else
#define MW_DMA_S2MM_IRQ  0x1e
#endif

#endif /* __ZYNQ_MW_AXISTREAM_COMMON__ */