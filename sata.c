#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "define.h"
#include "mapmemory.h"
#include "pci.h"
#include "sata.h"
#include "io.h"

extern u64 PCI_Base_Addr, StartBus, EndBus;
struct SATARootPort SataPort[16];

void SATATopology(int type, int decode)
{
	FILE *fptr;
	int SATAmode = UNKNOW_MODE, PortItemNum=0, i,j,k,x, PortCount,PortCount1;
	u32 address, value, ClassCode, ABAR, Speed, AHCIVersion,temp2;
	char buffer[BUFFER_SIZE], PortItem[BUFFER_SIZE][BUFFER_SIZE], *token;
	char path[128]={};
	
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
					ClassCode=bit_range(value, 31, 16);
					
					if(ClassCode == 0x106)				// AHCI mode
					{
						SATAmode=AHCI_MODE;
						
						address = get_pci_memory_addr(PCI_Base_Addr, i, j, k, 0x24);
						ABAR=(MEMRW(READ, address, 4, 0) & 0xfffffff0);
						if(debug >= 3)
							printf("[Debug] ABAR = %x\n", ABAR);
						
						value=MEMRW(READ, ABAR, 4, 0);
						PortCount1=bit_range(value, 4, 0);
                                                PortCount1=PortCount1+1;
						Speed=bit_range(value, 23, 20);
						AHCIVersion=MEMRW(READ, (ABAR+0x10), 4, 0);


						PortCount =0 ;
                                              /*  
                                                for(x=0; x<=7; x++)
                                                 {
                                                  temp2 = MEMRW(READ, ((ABAR + 0x100) + (x * 0x80) + 0x28), 4, 0); //kimi
                                                  if ((temp2 != 0xFFFFFFFF))   
	                                               {
                                                        PortCount=PortCount+1;		
                                                       }
                                                 }
                                              */
                                         

						printf("Support AHCI %x.%x specification\n", ((AHCIVersion & 0xf0000) >> 16), ((AHCIVersion & 0xf) + ((AHCIVersion & 0xf00) >> 4)));
						printf("%-25s%s%d\n", "Support maximum speed", " = Gen ",Speed);
						printf("%-25s%s%d\n", "Number of ports", " = ", PortCount1);
						
						path[0]='\0';
						strcat(path,PLATFORM_PATH);
						if(type == NORMAL_TYPE)
							strcat(path,SATA_ITEM);
						else
							strcat(path,SATA_ERROR_ITEM);
						
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
						
						SATATopologyOutput((ABAR + 0x100), PortCount1, SATAmode, PortItem, PortItemNum, type, decode);
					}
				}
			}
		}
	}
	
	if(SATAmode == UNKNOW_MODE)
		printf("No AHCI controller is found\n");
}

void SATATopologyOutput(u32 PortBaseAddr, int PortCount, int SATAmode, char PortItem[][BUFFER_SIZE], int PortItemNum, int OutputType, int decode)
{
	FILE *fptr;
	int hi, lo, i, j,k, counter, PortItemOutput, title_output = FALSE, error_count;
	u32 value, address, temp, value_read, offset,temp1;
	char buffer[BUFFER_SIZE], title[BUFFER_SIZE], result[BUFFER_SIZE];
	char *token;
	char path[128]={}, name[32];

	strcat(path,PLATFORM_PATH);
	strcat(path,SATA_REGISTER);
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return;
	}

	k = PortCount;
	if(OutputType == NORMAL_TYPE)
	{
		title_output = TRUE;
		printf("================================================================================\n");
		printf("%-15s%s","SATA Port","|");
		//for(i=0; i<=PortCount; i++)
                for(i=0; i<=7; i++)
                 {
                   temp1 = MEMRW(READ, (PortBaseAddr + (i * 0x80) + 0x28), 4, 0);
                     if ((temp1 != 0xFFFFFFFF))  
	             { 	
                      printf("%-9d%s", i,"|");
                      k=k-1;	
                      if(k==0)
                         i=8;
                     }
                 }
		printf("\n================================================================================\n");
	}

        error_count = 0;
	for(i=0; i<PortItemNum; i++)
	{
		PortItemOutput = FALSE;
		//error_count = 0;
                k = PortCount;
		//for(j=0; j<=PortCount; j++)
                for(j=0; j<=7; j++)
		{
                    temp1 = MEMRW(READ, (PortBaseAddr + (j * 0x80) + 0x28), 4, 0);
                     if ((temp1 != 0xFFFFFFFF))
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
						else if(counter == 2)				// type (AHCI/IDE)
						{
							if(!strcmp(token,"AHCI_MODE") && SATAmode == AHCI_MODE)
							{ }
							else if(!strcmp(token,"IDE_MODE") && SATAmode == IDE_MODE)
							{ }
							else if(!strcmp(token,"All"))
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
							if(SATAmode == AHCI_MODE)
								{
                                                                 temp=MEMRW(READ, (PortBaseAddr + (j * 0x80) + offset), 4, 0);
                                                                }
							else if(SATAmode == IDE_MODE)
							{/*
								address = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, bar);
								address=MEMRW(READ, address, 4, 0);
								address=address & 0xfff0;
								iorw(WRITE, address, offset, 4);
								temp=iorw(READ, (address+4), 0, 4);*/
							}
							value_read=bit_range(temp, hi, lo);
							if(value == value_read)
							{
								if(PortItemOutput == FALSE)
								{
									if(OutputType == NORMAL_TYPE)
										sprintf(title, "%-15s%s",PortItem[i],"|");
									else
										sprintf(title, "%-55s%s", PortItem[i],"|");
									PortItemOutput = TRUE;
								}
								if(OutputType == NORMAL_TYPE)
									sprintf(result, "%-9s|",name);
								else
								{
									if(!strcmp(name,"E"))
										{
                                                                                  error_count++;
                                                                                  //printf("error_count: %x\n",error_count);
                                                                                }
									sprintf(result, " %s |", name);
								}
								strcat(title,result);
								if(debug >= 3)
									printf("[Debug] Addr:%x hi:%x lo:%x value:%x\n",(PortBaseAddr + (j * 0x80) + offset),hi,lo,value);
								break;
							}
							counter = 7;
						}
						token = strtok(NULL, DELIMITERS_TYPE1);
					} //edn while 
				}//end if
                         
			}//end while
                      k=k-1;	
                      if(k==0)
                         j=8;
                     }// end if
                  
		}//end for
	
		if(OutputType == ERROR_TYPE && title_output == FALSE && (error_count > 0 || decode == TRUE))
		{ 
			title_output = TRUE;
			printf("================================================================================\n");
			printf("%-55s%s","SATA Port","|");
			//for(j=0; j<=PortCount; j++)
                        k = PortCount;
                         for(j=0; j<=7; j++)
                          {
                           temp1 = MEMRW(READ, (PortBaseAddr + (j * 0x80) + 0x28), 4, 0);
                           if ((temp1 != 0xFFFFFFFF))  
	                     {	
				printf(" %d %s",j,"|");
                                k=k-1;	
                                if(k==0)
                                j=8;
                             }
                          }
			printf("\n================================================================================\n");
		}
		if((decode == FALSE && error_count > 0) || decode == TRUE)
			printf("%s\n",title);
		
	}
	fclose(fptr);
	if(title_output == TRUE)
		printf("================================================================================\n");
	else if(OutputType == ERROR_TYPE && title_output == FALSE)
		printf("SATA Error Status: PASS\n");
        
        if(OutputType == ERROR_TYPE && title_output == TRUE)
                printf("SATA Error Status: FAIL\n");
	
}

void SATAOnOff(int OnOff, int port)
{
	FILE *fptr;
	u32 value, SetValue, addr, read_value;
	char buffer[BUFFER_SIZE];
	char *token;
	char path[128]={};
	int  counter, bus, dev, fun, offset, hi, lo;
	
	if(SB_PCI_DEFINE[0]!='\0')
	{
		if(debug > 3)
			printf("[Debug] SB:%s\n", SB_PCI_DEFINE);
	
		strcat(path,PLATFORM_PATH);
		strcat(path,SATA_ONOFF);
		
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
						if(!strcmp(token,"On") && OnOff==SATAON)
						{
						}
						else if(!strcmp(token,"Off") && OnOff==SATAOFF)
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
					else if(counter == 5)				// offset
					{
						offset = strtol(token, NULL, 16);
						counter = 6;
					}
					else if(counter == 6)				// high bit
					{
						hi = strtol(token, NULL, 10);
						counter = 7;
					}
					else if(counter == 7)				// low bit
					{
						lo = strtol(token, NULL, 10);
						counter = 8;
					}
					else if(counter == 8)				// value
					{
						value = strtol(token, NULL, 16);
						addr = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, offset);
						read_value=MEMRW(READ, addr, 4, 0);
						SetValue=set_bit(read_value, hi, lo, value);
						MEMRW(WRITE, addr, 4, SetValue);
						counter = 9;
					}
					token = strtok(NULL, DELIMITERS_TYPE1);
				}
			}
		}
		fclose(fptr);
	}
}