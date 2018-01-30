#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "define.h"
#include "mapmemory.h"
#include "rdmsr.h"
#include "cache.h"

struct MTRR FixedRangeMTRR[] =  {{0x250, 0x10000}, 
								{0x258, 0x4000}, 
								{0x259, 0x4000},
								{0x268, 0x1000},
								{0x269, 0x1000},
								{0x26a, 0x1000},
								{0x26b, 0x1000},
								{0x26c, 0x1000},
								{0x26d, 0x1000},
								{0x26e, 0x1000},
								{0x26f, 0x1000}};



void CacheEmrr()
{
uint64_t MSRvalue;
//int FixedDIMM[] =  {0x425,0x429,0x42d,0x431,0x435,0x439,0x43d,0x441};
int i,x;
x=0;

   for(i=0;i<22;i++)
     {
      MSRvalue = rdmsr(0x401+x);

       if (MSRvalue!=0)
         {
          printf("Bank %d detect ECC error.\n",i);
          printf("Bank %d register value 0x%05llx \n",i,MSRvalue);
         }
       x=x+4;
     }

}


void CacheMtrr()
{
	uint64_t IA32_MTRRCAP = rdmsr(0xfe);
	uint64_t IA32_MTRR_DEF_TYPE = rdmsr(0x2ff);
	uint64_t FixedSupported = BitRange64(IA32_MTRRCAP, 8, 8);
	uint64_t VariableSupported = BitRange64(IA32_MTRRCAP, 7, 0);
	uint64_t WCSupported = BitRange64(IA32_MTRRCAP, 10, 10);
	uint64_t DefaultMTRRMemoryType = BitRange64(IA32_MTRR_DEF_TYPE, 7, 0);
	uint64_t MTRREnable = BitRange64(IA32_MTRR_DEF_TYPE, 11, 11);
	uint64_t FixedMTRREnable = BitRange64(IA32_MTRR_DEF_TYPE, 10, 10);
	uint64_t address=0, num=sizeof(FixedRangeMTRR)/sizeof(FixedRangeMTRR[0]), MSRvalue, value;
	int i,j;
	unsigned int lowbit;
	
	if(FixedSupported == 1)
		printf("Fixed range MTRRs are supported\n");
	else
		printf("Fixed range MTRRs are not supported\n");
	printf("Variable range MTRRs is %#llu\n", VariableSupported);
	if(WCSupported == 1)
		printf("Write Combine memory type is supported\n");
	else
		printf("Write Combine memory type is not supported\n");
	
	if(MTRREnable == 0)
	{
		printf(" All MTRRs are disabled and the Uncacheable (UC) memory type is applied to all of physical memory\n");
		return;
	}
	if(FixedMTRREnable == 1)
	{
		printf("Fixed range MTRRs are enabled\n\n");
		
		printf("Memory type of fixed range MTRRs\n");
		printf("===================================================\n");
		printf("%-17s %s %s\n","Memory Address","|","Memory Type");
		printf("------------------+--------------------------------\n");
		for(i=0;i<num;i++)
		{
			MSRvalue = rdmsr(FixedRangeMTRR[i].MSRaddr);
			lowbit=0;
			for(j=0;j<8;j++)
			{
				value = BitRange64(MSRvalue, (lowbit+7), lowbit);
				printf("0x%05llx - 0x%05llx | ", address, address + (FixedRangeMTRR[i].size - 1));
				if(value == Uncacheable)
					printf("Uncacheable (UC)\n");
				else if(value == WriteCombining)
					printf("Write Combining (WC)\n");
				else if(value == WriteThrough)
					printf("Write-through (WT)\n");
				else if(value == WriteProtected)
					printf("Write-protected (WP)\n");
				else if(value == Writeback)
					printf("Writeback (WB)\n");
				address += FixedRangeMTRR[i].size;
				lowbit += 8;
			}
		}
		printf("===================================================\n");
	}

	//printf("%#llx,%#llx,%#llx,%#llx\n", IA32_MTRRCAP, FixedSupported, VariableSupported, WCSupported);
}

unsigned long long BitRange64(unsigned long long data, unsigned int highbit, unsigned int lowbit)
{
	unsigned int bits;
	
	bits = highbit-lowbit+1;
	data >>= lowbit;
	data &= (1ULL << bits)-1;
	return data;
}