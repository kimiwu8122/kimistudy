#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "define.h"
#include "mapmemory.h"
#include "pci.h"
#include "usb.h"
#include "io.h"

extern u64 PCI_Base_Addr, StartBus, EndBus;
struct USBRootPort USBPort[32];

void USBTopology(int mode, int type, int decode)
{

	FILE *fptr;
	u32 value, address,address_l,address_h,CAPLENGTH, ClassCode, MEM_BASE ,MEM_BASE_L,MEM_BASE_H;
	char buffer[BUFFER_SIZE], PortItem[BUFFER_SIZE][BUFFER_SIZE];
	char *token;
	int PortItemNum=0, i,j,k, PortCount, USBSupportCount=1;
	char path[128]={};
	
	int Ehci1, Ehci2, Xhci, Ehci1PortCount=-1, Ehci2PortCount=-1, XhciPortCount=-1;
        //printf("kimi1\n");
	for(i=StartBus; i<=EndBus; i++)
	{
		for(j=0; j<=0x1f; j++)
		{
			for(k=0; k<=0x7; k++)
			{
				address = get_pci_memory_addr(PCI_Base_Addr, i, j, k, 0);
				value=MEMRW(READ, address, 4, 0);
                       
				if(value != 0xffffffff && value != 0)
				{
					value=MEMRW(READ, (address+0x8), 4, 0);
					ClassCode=bit_range(value, 31, 8);
                                        //printf("ClassCode:%x \n",ClassCode);
					if( (ClassCode == 0xc0320 && mode == EHCI_MODE) || (ClassCode == 0xc0330 && mode == XHCI_MODE))	// EHCI mode || XHCI mode
					{ //  printf("kimi2\n");
                                           
						address = get_pci_memory_addr(PCI_Base_Addr, i, j, k, 0x10);
                                                
						MEM_BASE=(MEMRW(READ, address, 4, 0) & 0xfffffff0);
                                              
                                        
                                           
						if(debug >= 3)
							printf("[Debug] MEM_BASE = %x\n", MEM_BASE);
						
						value=MEMRW(READ, MEM_BASE, 4, 0);
						CAPLENGTH=bit_range(value, 7, 0);
						
						path[0]='\0';
						strcat(path,PLATFORM_PATH);
						
						if(mode == EHCI_MODE && type == NORMAL_TYPE)
						{
							strcat(path,USB_EHCI_ITEM);
							printf("\n%s%d%s%d%s%d%s\n", "EHXI controller (Bus:", i, ", Dev:", j, ", Fun:", k, ")");
							value=MEMRW(READ, (MEM_BASE+0x4), 4, 0);
							PortCount=bit_range(value, 3, 0);
							PortCount=PortCount-1;
						}
						else if (mode == XHCI_MODE)
						{
							if(type == NORMAL_TYPE)
								strcat(path,USB_XHCI_ITEM);
							else
								strcat(path,USB_ERROR_ITEM);
							printf("\n%s%d%s%d%s%d%s\n", "XHCI controller (Bus:", i, ", Dev:", j, ", Fun:", k, ")");
							value=MEMRW(READ, (MEM_BASE+0x4), 4, 0);
							PortCount=bit_range(value, 31, 24);
							PortCount=PortCount-1;
						}
						//printf("%-25s%s%d\n", "Number of ports", " = ", (PortCount+1)); //kimi
						
						fptr = fopen(path, "r");				// open file
						if(!fptr)
						{
							printf("\n[Error] Can not open %s\n", path);
							return 0;
						}
						fseek(fptr, 0, SEEK_SET);
						
						PortItemNum=0;
						while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
						{
							token = strtok(buffer, DELIMITERS_TYPE1);
							if(strchr(token, '#') == 0)
							{
								strncpy(PortItem[PortItemNum], token, sizeof(PortItem[PortItemNum]));
								PortItemNum++;
							}
						}
						fclose(fptr);
						if(mode == EHCI_MODE)
						{
							int OutputPortNum;
							for(OutputPortNum=0; OutputPortNum<PortCount;OutputPortNum=OutputPortNum+6)
								USBTopologyOutput((MEM_BASE + CAPLENGTH + 0x44), PortCount, mode, PortItem, PortItemNum, type, decode, OutputPortNum);
						}
						else if(mode == XHCI_MODE)
						{
							int OutputPortNum;
							for(OutputPortNum=0; OutputPortNum<PortCount;OutputPortNum=OutputPortNum+6)
							{
								if(type == NORMAL_TYPE)
									USBTopologyOutput((MEM_BASE + CAPLENGTH + 0x400), PortCount, mode, PortItem, PortItemNum, type, decode, OutputPortNum);
								else 
									USBTopologyOutput((MEM_BASE + CAPLENGTH + 0x408), PortCount, mode, PortItem, PortItemNum, type, decode, OutputPortNum);
								printf("\n");
							}
						}
					}
				}
			}
		}
	}
}

void USBTopologyOutput(u32 PortBaseAddr, int PortCount, int mode, char PortItem[][BUFFER_SIZE], int PortItemNum, int OutputType, int decode, int OutputPortNum)
{
	FILE *fptr;
	int hi, lo, i, j, counter, PortItemOutput, title_output = FALSE, error_count, FirstOutput,error_count1;
	u32 value, address, temp, value_read, offset;
	char buffer[BUFFER_SIZE], title[BUFFER_SIZE], result[BUFFER_SIZE];
	char *token;
	char path[128]={}, name[32];
	
	strcat(path,PLATFORM_PATH);
	strcat(path,USB_REGISTER);
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{printf("kimi2\n");
		printf("\n[Error] Can not open %s\n", path);
		return;
	}
	
	FirstOutput = TRUE;
	if(OutputType == NORMAL_TYPE)
	{
		title_output = TRUE;
		printf("================================================================================\n");
		printf("%-15s%s","USB Port","|");
		for(i=OutputPortNum; i<=PortCount; i++)
		{
			if(FirstOutput == TRUE || i%6!=0)
			{
			       FirstOutput=FALSE;

                               if(i >= 15)
                               {
                                printf("XHCI %-5d%s", i-15 ,"|"); //kimi
                               } 
                               else
                               {
				printf("%-9d%s", i,"|");
                               }
			}
			else
				break;
		}

                
		printf("\n================================================================================\n");
	}
	error_count1=0; //kimi
	for(i=0; i<PortItemNum; i++)
	{
		PortItemOutput = FALSE;
		error_count = 0;
		
		FirstOutput = TRUE;
		for(j=OutputPortNum; j<=PortCount; j++)
		{
		if(FirstOutput == TRUE || j%6!=0)
		{
			FirstOutput=FALSE;
			fseek(fptr, 0, SEEK_SET);
			while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
			{
				token = strtok(buffer, DELIMITERS_TYPE1);
				if(strchr(token, '#') == 0)
				{
					counter = 0;
					while(token != NULL)
					{
						if(counter == 0)					// item name
						{
							if(strcmp(token,PortItem[i]))
								break;
							counter = 1;
						}
						else if(counter == 1)				// name
						{
							name[0]='\0';
							strncpy(name, token, sizeof(name));
							counter = 2;
						}
						else if(counter == 2)				// type (EHCI/XHCI)
						{
							if(!strcmp(token,"EHCI_MODE") && mode == EHCI_MODE)
							{ }
							else if(!strcmp(token,"XHCI_MODE") && mode == XHCI_MODE)
							{ }
							else
								break;
							counter = 3;
						}
						else if(counter == 3)				// offset
						{
							offset = strtol(token, NULL, 16);
							counter = 4;
						}
						else if(counter == 4)				// high bit
						{
							hi = strtol(token, NULL, 10);
							counter = 5;
						}
						else if(counter == 5)				// low bit
						{
							lo = strtol(token, NULL, 10);
							counter = 6;
						}
						else if(counter == 6)				// value
						{
							value = strtol(token, NULL, 16);
							if(mode == EHCI_MODE || mode == XHCI_MODE)
								temp=MEMRW(READ, (PortBaseAddr + (j * offset)), 4, 0);
							value_read=bit_range(temp, hi, lo);
							if(value == value_read || OutputType == ERROR_TYPE)
							{
								if(PortItemOutput == FALSE)
								{
									if(OutputType == NORMAL_TYPE)
										sprintf(title, "%-15s%s",PortItem[i],"|");
									else
										sprintf(title, "%-25s%s", PortItem[i],"|");
									PortItemOutput = TRUE;
								}
								if(OutputType == NORMAL_TYPE)
									sprintf(result, "%-9s|",name);
								else
								{
									if(value_read != 0)
										{
                                                                                 error_count++;
                                                                                 error_count1++;
                                                                                // printf("Error_count1:%d\n",error_count1);
                                                                                }
									sprintf(result, " %4d |", value_read);
								}
								strcat(title,result);
								if(debug >= 3)
									printf("[Debug] Addr:%x hi:%x lo:%x value:%x, %x\n",(PortBaseAddr + (j * offset)),hi,lo,value, temp);
								break;
							}
							counter = 7;
						}
						token = strtok(NULL, DELIMITERS_TYPE1);
					}
				}
			}
		}
		else
			break;
		}
		
		if(OutputType == ERROR_TYPE && title_output == FALSE && (error_count > 0 || decode == TRUE))
		{      
                        title_output = TRUE;
                        FirstOutput = TRUE;
			printf("================================================================================\n");
			printf("%-25s%s","USB Port","|");
			for(j=OutputPortNum; j<=PortCount; j++)
			{
				if(FirstOutput == TRUE || j%6!=0)
				{
					FirstOutput=FALSE; 
                                        if(j >= 15)
                                         {
                                          printf("XHCI%-2d%s", j-15 ,"|"); //kimi
                                         } 
                                        else
                                          {
					   printf(" %4d %s",j,"|"); 
                                          }
				}
				else
					break;
			}
			printf("\n================================================================================\n");
		}
		if((decode == FALSE && error_count > 0) || decode == TRUE)
			printf("%s\n",title);
		
	}
	fclose(fptr);
	//if(title_output == TRUE)
		printf("================================================================================\n");
	//else if(OutputType == ERROR_TYPE && title_output == FALSE)
         if(OutputType == ERROR_TYPE && (error_count1 > 0))
		printf("USB Error Status: FAIL\n");
}

void USBOnOff(int OnOff, int port)
{
	FILE *fptr;
	u32 value, PMBASE, SetValue, addr, read_value;
	char buffer[BUFFER_SIZE];
	char *token;
	char path[128]={};
	int  counter, bus, dev, fun, bar, offset, hi, lo;
	
	if(SB_PCI_DEFINE[0]!='\0')
	{
		if(debug > 3)
			printf("[Debug] SB:%s\n", SB_PCI_DEFINE);
	
		// Set "USB Per-Port Registers Write Enable" to enable (PMBASE +3Ch)
		addr = get_pci_memory_addr(PCI_Base_Addr, 0, 0x1f, 0, 0x40);
		PMBASE=MEMRW(READ, addr, 4, 0);
		PMBASE=PMBASE & 0xfff0;
		read_value=iorw(READ, (PMBASE + 0x3c), 0, 1);
		SetValue=set_bit(read_value, 1, 1, 1);
		iorw(WRITE, (PMBASE + 0x3c), SetValue, 1);
		
		strcat(path,PLATFORM_PATH);
		strcat(path,USB_ONOFF);
		
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
				counter = 0;
				while(token != NULL)
				{
					if(counter == 0)					// method
					{
						if(strcmp(token, SB_PCI_DEFINE))
							break;
						counter = 1;
					}
					else if(counter == 1)				// On / Off
					{
						if(!strcmp(token,"On") && OnOff==USBON)
						{
						}
						else if(!strcmp(token,"Off") && OnOff==USBOFF)
						{
						}
						else
							break;
						counter = 2;
					}
					else if(counter == 2)				// bus
					{
						bus = strtol(token, NULL, 16);
						counter = 3;
					}
					else if(counter == 3)				// dev
					{
						dev = strtol(token, NULL, 16);
						counter = 4;
					}
					else if(counter == 4)				// fun
					{
						fun = strtol(token, NULL, 16);
						counter = 5;
					}
					else if(counter == 5)				// bar
					{
						bar = strtol(token, NULL, 16);
						counter = 6;
					}
					else if(counter == 6)				// offset
					{
						offset = strtol(token, NULL, 16);
						counter = 7;
					}
					else if(counter == 7)				// high bit
					{
						hi = strtol(token, NULL, 10);
						counter = 8;
					}
					else if(counter == 8)				// low bit
					{
						lo = strtol(token, NULL, 10);
						counter = 9;
					}
					else if(counter == 9)				// value
					{
						value = strtol(token, NULL, 16);
						if(bar > 0)
							addr = get_pci_mmio_addr(PCI_Base_Addr, bus, dev, fun, bar, offset);
						else
							addr = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, offset);
						read_value=MEMRW(READ, addr, 4, 0);
						SetValue=set_bit(read_value, hi, lo, value);
						if(debug > 3)
							printf("[Debug] addr:%x, read_value:%x, SetValue:%x\n", addr, read_value, SetValue);
						MEMRW(WRITE, addr, 4, SetValue);
						counter = 10;
					}
					token = strtok(NULL, DELIMITERS_TYPE1);
				}
			}
		}
		fclose(fptr);
	}
	
	
}
