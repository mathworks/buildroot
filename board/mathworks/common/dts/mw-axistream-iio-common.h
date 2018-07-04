#ifndef __MW_AXISTREAM_COMMON__
#define __MW_AXISTREAM_COMMON__

#if defined(MW_DATAWIDTH_SELECT) && MW_DATAWIDTH_SELECT == 64
	#define MW_MM2S_DATAFMT "u64/64>>0"
	#define MW_S2MM_DATAFMT "u64/64>>0"
#else
	#define MW_MM2S_DATAFMT "u32/32>>0"
	#define MW_S2MM_DATAFMT "u32/32>>0"
#endif

#endif /* __MW_AXISTREAM_COMMON__ */
