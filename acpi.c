#include <stdio.h>

#include "define.h"
#include "mapmemory.h"

#define RSDP_SIZE		16

/* find address by the signature */
u64 *find_address(u8 *begin, u32 length, char *string)
{
	u8 *i, *end = begin + length;

	for (i = begin; i < end; i += RSDP_SIZE)
	{
		if (!(memcmp((void *)i, (void *)string, strlen(string))))	// check the signature
		{//printf("\nCan not find RSDP\n");
			return (i-begin);					// return address
		}
	}
	
	return 0;	// can not find
}

u64 MCFG_Base_Address(u32 RSDP_START_ADDR, u32 RSDP_END_ADDR, int Return_EndBus)
{
	u8 *map_addr;										//map memory use
	u32 *RSDP_RSDT_ADDR, *RSDT_Length;					//record RSDP Addr pointer & RSDT length pointer
	u32 RSDT_ADDR, MCFG_ADDR;							//record RSDT start address & MCFG start address
	u32 RSDT_size, MCFG_size=0;							//RSDT/MCFG table size
	u64 MCFG_BASE_ADDR;									//MCFG base address (return value)
	u32 offset;											//record the offset of RSDP from RSDP_START_ADDR
	u32 addr, length;									//record RSDP_START_ADDR & RSDP_END_ADDR
	char *RSDPTR_SIG="RSD PTR ";						// RSDP PTR signature
	int End_Bus_offset=11;
	u32 *Bus_Number=malloc(sizeof(unsigned int));	//
	u32 End_Bus_Number;

	/* ACPI */
	addr = RSDP_START_ADDR;
	length = RSDP_END_ADDR;
	
	/* Find RSDP */
	if (!(map_addr = map_memory('r', addr, length)) || !(offset = find_address(map_addr, length, RSDPTR_SIG)))
	{
		unmap_memory(map_addr, length);
		return 0;
	}
	if(debug)
		printf ("RSDPTR Offset=%x\n", offset);				//Debug
	
	//Find RSDT ADRRESS in RSDP structure
	RSDP_RSDT_ADDR= map_addr+offset+RSDP_SIZE;          	//RSDT address locate at RSDP structure offset 0x10
	RSDT_ADDR=*RSDP_RSDT_ADDR;								//Get RSDT address
	if(debug)
		printf ("RSDT address %x\n", *RSDP_RSDT_ADDR); 		//Debug            
	unmap_memory(map_addr, length);

	if (!(map_addr = map_memory('r', RSDT_ADDR, 32))) 		//Check RSDT header
	{
		printf("RSDT memory map FAIL\n");
		unmap_memory(map_addr, 32);
		return 0;
	}
	RSDT_Length = map_addr+4;								//Check RSDT table size
	RSDT_size=*RSDT_Length;
	if(debug)
		printf("RSDT size =%x\n", *RSDT_Length);			//Debug
	unmap_memory(map_addr, 32);
	
	if (!(map_addr = map_memory('r', RSDT_ADDR, RSDT_size))) //Check RSDT Table
	{
		printf("RSDT memory map FAIL\n");
		unmap_memory(map_addr, RSDT_size);
		return 0;
	}
	
	int j;													//for loop to find every Entry in RSDT table
	for(j=36;j<RSDT_size;j+=4)                           	//1 Entry :4bytes, Entry start at RSDP offset 36
	{
		unsigned char *Search_Entry, *map_addr1; 			//in-search address & new mapping memory
		unsigned int temp_addr;								//code use to record the address
		char Entry_Name[5]="1234";							//String for Entry
		
		Search_Entry=map_addr+j;							//Entry in memory location
		temp_addr=*(unsigned int *)Search_Entry;			//Entry address
		if(debug)
			printf("%x, %x, %x, Entry address=%x\n",map_addr, j, Search_Entry, temp_addr);			//Debug
		if (!(map_addr1 = map_memory('r', temp_addr, 32)))	//Check Entry header
		{
			printf("Entry memory map FAIL\n");
			unmap_memory(map_addr1, 32);
			return 0;
		}
		memcpy((void*)Entry_Name,(void*) map_addr1, 4);		//Entry Signature:4bytes, copy Entry name
		Entry_Name[4]='\0';
		if(debug)
		{
			printf("Entry=%s\n", Entry_Name);				//Debug
		}
		if (!(strcmp(Entry_Name,"MCFG")))					// Check the signature is MCFG or not
		{ 
			if(debug)
				printf ("Find the MCFG successfully\n");	//Debug
			MCFG_ADDR=temp_addr;
			MCFG_size=*(unsigned int*)(map_addr1+4);      	//MCFG Table Length at MCFG table offset 4
			if(debug)
				printf ("MCFG size=%x\n",MCFG_size);
			break;
		}
	    unmap_memory(map_addr1, 32);		
	}
	unmap_memory(map_addr, RSDT_size);

	if (!(map_addr = map_memory('r', MCFG_ADDR, MCFG_size)))	//Check MCFG Table
	{
		printf("MCFG memory map FAIL\n");
		unmap_memory(map_addr, MCFG_size);
		return 0;
	}			
	//+Ryan Debug
	if(debug)
	{
		unsigned int * temp1;
		unsigned int temp2;
		printf("map_addr=%x\n",map_addr);
		temp1=map_addr+44;
		printf("MCFG_BASE_ADDR_offset=%x\n",temp1);
		temp2=*temp1;
		printf("MCFG_BASE_ADDR=%x\n",temp2);
	}
	
	MCFG_BASE_ADDR=*(unsigned long*)(map_addr+44);			//Base address offset 44 in MCFG table
	Bus_Number=map_addr+44+End_Bus_offset;				//End Bus number offset 11
	End_Bus_Number=*Bus_Number;
	unmap_memory(map_addr, MCFG_size);
	if(debug)
	{
		printf("MCFG_BASE_ADDR=%lx \n",MCFG_BASE_ADDR);		//Debug
		printf("End_Bus_Number=%x\n",End_Bus_Number);
	}
	
	if(Return_EndBus)
		return End_Bus_Number;
	else
		return MCFG_BASE_ADDR;
}

