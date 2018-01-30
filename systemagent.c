#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "define.h"
#include "mapmemory.h"
#include "pci.h"
#include "systemagent.h"
#include "io.h"

extern u64 PCI_Base_Addr;
struct SARootPort SAPort[16];

int SystemAgentPortDefine(char SADefine[BUFFER_SIZE])
{
	FILE *fptr;
	char buffer[BUFFER_SIZE], *token;
	int i=0;
	
	char path[128]={};
	strcat(path,PLATFORM_PATH);
	strcat(path,SYSTEM_AGENT_SUPPORT);
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return;
	}
	fseek(fptr, 0, SEEK_SET);

	while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
	{
		token = strtok(buffer, DELIMITERS_TYPE1);

		if(strchr(token, '#') == 0)
		{
			if(!strcmp(token, SADefine))
			{
				token = strtok(NULL, DELIMITERS_TYPE3);
				while(token != NULL)
				{
					strncpy((SAPort+i)->name, token, sizeof((SAPort+i)->name));
					i++;
					token = strtok(NULL, DELIMITERS_TYPE3);
				}
			}
		}
	}
	
	fclose(fptr);
	return i;
}

void SystemAgent()
{
	FILE *fptr;
	int SAItemNum=0;
	char buffer[BUFFER_SIZE], SAItem[BUFFER_SIZE][BUFFER_SIZE];
	char *token;
	
	if(NB_PCI_DEFINE[0]!='\0')
	{
		if(debug > 3)
			printf("[Debug] NB:%s\n", NB_PCI_DEFINE);
		char path[128]={};
		strcat(path,PLATFORM_PATH);
		strcat(path,SYSTEM_AGENT_ITEM);
		fptr = fopen(path, "r");				// open file
		if(!fptr)
		{
			printf("\n[Error] Can not open %s\n", path);
			return 0;
		}
		fseek(fptr, 0, SEEK_SET);
		
		while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
		{
			token = strtok(buffer, DELIMITERS_TYPE1);
			if(strchr(token, '#') == 0)
			{
				strncpy(SAItem[SAItemNum], token, sizeof(SAItem[SAItemNum]));
				SAItemNum++;
			}
		}
		fclose(fptr);
		
		SystemAgentOutput(SAItem, SAItemNum);

	}
}

void SystemAgentOutput(char SAItem[][BUFFER_SIZE], int SAItemNum)
{
	FILE *fptr;
	int counter = 0, i=0, j, bus, dev, fun, bar, offset, hi, lo, type, SAItemOutput;
	u32 value, address, temp, value_read, PortCount;
	char buffer[BUFFER_SIZE], name[32], string[BUFFER_SIZE];
	char *token, *token1;
	char path[128]={};
	
	PortCount=SystemAgentPortDefine(NB_PCI_DEFINE);
	if(debug > 3)
		printf("[Debug] PortCount:%d\n", PortCount);
	if(PortCount == 0)
		return PortCount;
	
	printf("================================================================================\n");
	printf("%-20s|","System Agent");
	for(i=0; i<PortCount; i++)
		printf("%-25s%s",(SAPort+i)->name,"|");
	printf("\n--------------------------------------------------------------------------------\n");

	strcat(path,PLATFORM_PATH);
	strcat(path,SYSTEM_AGENT_REGISTER);
	
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return;
	}
	fseek(fptr, 0, SEEK_SET);
	
	
	for(i=0; i<SAItemNum; i++)
	{
		SAItemOutput = FALSE;
		for(j=0; j<PortCount; j++)
		{
			fseek(fptr, 0, SEEK_SET);
			while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
			{
				token = strtok(buffer, DELIMITERS_TYPE1);
				if(strchr(token, '#') == 0)
				{
					counter = 0;
					while(token != NULL)
					{
						if(counter == 0)					// port name
						{
							if(strcmp(token,(SAPort+j)->name))
								break;
							counter = 1;
						}
						else if(counter == 1)				// item name
						{
							if(strcmp(token,SAItem[i]))
								break;
							counter = 2;
						}
						else if(counter == 2)				// name
						{
							name[0]='\0';
							strncpy(name, token, sizeof(name));
							counter = 3;
						}
						else if(counter == 3)				// NB method (SNB_Server_Method/SNB_Client_Method/HSW_Client_Method)
						{
							if(strcmp(token,NB_PCI_DEFINE))
								break;
							counter = 4;
						}
						else if(counter == 4)				// address/bar offset
						{
							if(!strcmp(token,"address"))
								type = ADDRESS_TYPE;
							else if(!strcmp(token,"MMIO"))
								type = MMIO_TYPE;
							else if(!strcmp(token,"IO"))
								type = IO_TYPE;
							else
								break;
							counter = 5;
						}
						else if(counter == 5)				// bus
						{
							bus = strtol(token, NULL, 16);
							counter = 6;
						}
						else if(counter == 6)				// dev
						{
							dev = strtol(token, NULL, 16);
							counter = 7;
						}
						else if(counter == 7)				// fun
						{
							fun = strtol(token, NULL, 16);
							counter = 8;
						}
						else if(counter == 8)				// bar
						{
							bar = strtol(token, NULL, 16);
							counter = 9;
						}
						else if(counter == 9)				// offset
						{
							offset = strtol(token, NULL, 16);
							counter = 10;
						}
						else if(counter == 10)				// high bit
						{
							hi = strtol(token, NULL, 10);
							counter = 11;
						}
						else if(counter == 11)				// low bit
						{
							lo = strtol(token, NULL, 10);
							counter = 12;
						}
						else if(counter == 12)				// value
						{
							value = strtol(token, NULL, 16);
							if(type == ADDRESS_TYPE)
							{
								address = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, offset);
								temp=MEMRW(READ, address, 4, 0);
							}
							else if(type == MMIO_TYPE)
							{
								address = get_pci_mmio_addr(PCI_Base_Addr, bus, dev, fun, bar, offset);
								temp=MEMRW(READ, address, 4, 0);
							}
							else if(type == IO_TYPE)
							{
								address = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, bar);
								address=MEMRW(READ, address, 4, 0);
								address=address & 0xfff0;
								iorw(WRITE, address, offset, 4);
								temp=iorw(READ, (address+4), 0, 4);
							}
							value_read=bit_range(temp, hi, lo);
							
							if(value == value_read)
							{
								if(SAItemOutput == FALSE)
								{
									printf("%-20s%s",SAItem[i],"|");
									SAItemOutput = TRUE;
								}
								printf("%-25s|",name);
								if(debug > 1)
									printf("[Debug] Type:%d, Bus:%x Dev:%x Fun:%x Bar:%x Offset:%x hi:%x lo:%x value:%x\n",type,bus,dev,fun,bar,offset,hi,lo,value);
								break;
							}
							if(debug > 2)
								printf("[Debug] Type:%d, B:%x D:%x F:%x Bar:%x Offset:%x hi:%x lo:%x value:%x addr:%x read:%x\n",type,bus,dev,fun,bar,offset,hi,lo,value,address,value_read);
							counter = 13;
						}
						token = strtok(NULL, DELIMITERS_TYPE1);
					}
				}
			}
		}
		if(SAItemOutput == TRUE)
			printf("\n");
	}
	fclose(fptr);
	printf("================================================================================\n");
}