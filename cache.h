#ifndef __CACHE_H__
#define __CACHE_H__

#define Uncacheable			0
#define WriteCombining		1
#define WriteThrough		4
#define WriteProtected		5
#define Writeback			6

struct MTRR {
	int MSRaddr;		// MSR address
	int size;			// size
};

struct DIMM {
	int MSRaddr;		// MSR address
};

void CacheMtrr();	// Memory Type Range Registers
void CacheEmrr();	// System Management Range Register
void CacheSmrr();	// Processor Reserved Range Register
unsigned long long BitRange64(unsigned long long value, unsigned int highbit, unsigned int lowbit);



#endif						/* __CACHE_H__ */
