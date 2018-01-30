#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "define.h"
#include "mapmemory.h"
#include "pci.h"
#include "systemagent.h"

u64 PCI_Base_Addr, StartBus, EndBus;
struct PciRootPort PciPort[128];
struct PciDeviceCount Device[128];
struct PciDeviceStatus DeviceStatus[BUFFER_SIZE];
struct PciError PciErrorItem[BUFFER_SIZE];

void GetPCIBaseAddress()
{
	PCI_Base_Addr = MCFG_Base_Address(RSDP_START_ADDR, RSDP_END_OFFSET, 0);
	StartBus = 0;
	EndBus = MCFG_Base_Address(RSDP_START_ADDR, RSDP_END_OFFSET, 1);
	
	if(PCI_Base_Addr == 0)
	{
		printf("[Error] Can't find base address in MCFG table\n");
		exit(1);
	}
	
	if(debug > 0)
		printf("[Debug] PCI_Base_Addr:%x StartBus:%x EndBus:%x\n",PCI_Base_Addr, StartBus, EndBus);
}

void CSRread(int bus, int dev, int fun, int offset)
{
	int i, end = offset+0xfc, addr;
	u32 address, value, temp;
	
	GetPCIBaseAddress();

	printf("===================================================\n");
	printf("%03x|00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n", offset);
	printf("---------------------------------------------------");
	for(i=offset; i<=end ;i=i+4)
	{
		address = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, i);
		value=MEMRW(READ, address, 4, 0);
		
		if(i%16 == 0)
		{
			addr = i % 0x100;
			printf("\n%02X |", addr);
		}
		temp=bit_range(value, 7, 0);
		printf("%02X ",temp);
		temp=bit_range(value, 15, 8);
		printf("%02X ",temp);
		temp=bit_range(value, 23, 16);
		printf("%02X ",temp);
		temp=bit_range(value, 31, 24);
		printf("%02X ",temp);
	}
	printf("\n===================================================\n");
}

int ChipsetSupport()
{
	FILE *fptr;
	char buffer[BUFFER_SIZE], name1[BUFFER_SIZE], name2[BUFFER_SIZE];
	char *token, *temp;
	int counter;
	u32 VendorID, DeviceID, NB_VID=0, NB_DID=0, SB_VID=0, SB_DID=0, address, value;;
	NB_PCI_DEFINE[0]='\0', SB_PCI_DEFINE[0]='\0';
	
	address = get_pci_memory_addr(PCI_Base_Addr, 0, 0, 0, 0);
	value=MEMRW(READ, address, 4, 0);
	NB_VID=bit_range(value, 15, 0);
	NB_DID=bit_range(value, 31, 16);

	address = get_pci_memory_addr(PCI_Base_Addr, 0, 0x1f, 0, 0);
	value=MEMRW(READ, address, 4, 0);
	SB_VID=bit_range(value, 15, 0);
	SB_DID=bit_range(value, 31, 16);
	
	if(debug > 1)
	{
		printf("[Debug] North Bridge VID:%x, DID:%x\n",NB_VID, NB_DID);
		printf("[Debug] South Bridge VID:%x, DID:%x\n",SB_VID, SB_DID);
	}
	
	char path[128]={};
	strcat(path,PLATFORM_PATH);
	strcat(path,SUPPORT_NB);
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return 0;
	}
	fseek(fptr, 0, SEEK_SET);
	
	while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
	{
		counter = 0;
		token = strtok(buffer, DELIMITERS_TYPE1);

		if(strchr(token, '#') == 0)
		{
			while(token != NULL)
			{
				if(counter == 0)					// VID
				{
					VendorID = strtol(token, NULL, 16);
					counter = 1;
				}
				else if(counter == 1)				// DID
				{
					DeviceID = strtol(token, NULL, 16);
					counter = 2;
				}
				else if(counter == 2)				// NB Name
				{
					strncpy(name1, token, BUFFER_SIZE);
					counter = 3;
				}
				else if(counter == 3)				// Method
				{
					if((VendorID == NB_VID) && (DeviceID == NB_DID))
					{
						strncpy(NB_PCI_DEFINE, token, BUFFER_SIZE);
						temp = strtok(NB_PCI_DEFINE, DELIMITERS_TYPE1);
						strncpy(NB_PCI_DEFINE, temp, BUFFER_SIZE);
						break;
					}
					counter = 4;
				}
				token = strtok(NULL, DELIMITERS_TYPE2);
			}
			
			if((VendorID == NB_VID) && (DeviceID == NB_DID))
				break;
		}
	}
	fclose(fptr);
	
	path[0]='\0';
	strcat(path,PLATFORM_PATH);
	strcat(path,SUPPORT_SB);
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return 0;
	}
	fseek(fptr, 0, SEEK_SET);
	
	while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
	{
		counter = 0;
		token = strtok(buffer, DELIMITERS_TYPE1);

		if(strchr(token, '#') == 0)
		{
			while(token != NULL)
			{
				if(counter == 0)					// VID
				{
					VendorID = strtol(token, NULL, 16);
					counter = 1;
				}
				else if(counter == 1)				// DID
				{
					DeviceID = strtol(token, NULL, 16);
					counter = 2;
				}
				else if(counter == 2)				// Name
				{
					strncpy(name2, token, BUFFER_SIZE);
					counter = 3;
				}
				else if(counter == 3)				// Method
				{
					if((VendorID == SB_VID) && (DeviceID == SB_DID))
					{
						strncpy(SB_PCI_DEFINE, token, BUFFER_SIZE);
						temp = strtok(SB_PCI_DEFINE, DELIMITERS_TYPE1);
						strncpy(SB_PCI_DEFINE, temp, BUFFER_SIZE);
						break;
					}
					counter = 4;
				}
				token = strtok(NULL, DELIMITERS_TYPE2);
			}
			
			if((VendorID == SB_VID) && (DeviceID == SB_DID))
				break;
		}
	}	
	
	fclose(fptr);
	
	if(NB_PCI_DEFINE[0]=='\0')
		printf("The North Bridge is not supported. VID:%x, DID:%x\n",NB_VID, NB_DID);
	if(SB_PCI_DEFINE[0]=='\0')
		printf("The South Bridge is not supported. VID:%x, DID:%x\n",SB_VID, SB_DID);
	
	if(debug > 0 && NB_PCI_DEFINE[0]!='\0')
		printf("[Debug] The North Bridge is%s\n",name1);
	if(debug > 0 && SB_PCI_DEFINE[0]!='\0')
		printf("[Debug] The South Bridge is%s\n",name2);

    return 0;
}

void PciTopology()
{
	FILE *fptr;
	char buffer[BUFFER_SIZE], *token, PortItem[BUFFER_SIZE][BUFFER_SIZE], DeviceItem[BUFFER_SIZE][BUFFER_SIZE];
	int PortItemNum=0, DeviceItemNum=0;
	
	if(NB_PCI_DEFINE[0]=='\0' && SB_PCI_DEFINE[0]=='\0')
		return;
		
	char path[128]={};
	strcat(path,PCI_PATH);
	strcat(path,ROOTPORT_CONFIG);
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
			strncpy(PortItem[PortItemNum], token, sizeof(PortItem[PortItemNum]));
			PortItemNum++;
		}
	}
	fclose(fptr);
	
	path[0]='\0';
	strcat(path,PCI_PATH);
	strcat(path,DEVICE_CONFIG);
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
			strncpy(DeviceItem[DeviceItemNum], token, sizeof(DeviceItem[DeviceItemNum]));
			DeviceItemNum++;
		}
	}
	fclose(fptr);
	
	path[0]='\0';
	strcat(path,PCI_PATH);
	strcat(path,REGISTER_CONFIG);
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return 0;
	}
	
	PciTopologyOutput(NB_PCI_DEFINE, PortItem, DeviceItem, PortItemNum, DeviceItemNum, "CPU");
	PciTopologyOutput(SB_PCI_DEFINE, PortItem, DeviceItem, PortItemNum, DeviceItemNum, "PCH");

	fclose(fptr);
}

int PciPortDefine(char PortDefine[BUFFER_SIZE], char path[])
{
	FILE *fptr;
	char buffer[BUFFER_SIZE], *token;
	int counter, find = 1, num = 0, i=0;
	
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
			if(!strcmp(token, PortDefine))
			{
				token = strtok(NULL, DELIMITERS_TYPE3);
				counter = 0;
				while(token != NULL)
				{
					if(counter == 0)					// Port name
					{
						find = 2;
						strncpy((PciPort+i)->name, token, sizeof(PciPort->name));
						(PciPort+i)->count=num;
						counter = 1;
					}
					else if(counter == 1)				// bus
					{
						(PciPort+i)->bus = strtol(token, NULL, 16);
						counter = 2;
					}
					else if(counter == 2)				// dev
					{
						(PciPort+i)->dev = strtol(token, NULL, 16);
						counter = 3;
					}
					else if(counter == 3)				// fun
					{
						(PciPort+i)->fun = strtol(token, NULL, 16);
						counter = 0;
						if(debug > 1)
						printf("[Debug] i=%d,%s,%d,B=%x,D=%x,F=%x,\n",i,(PciPort+i)->name,(PciPort+i)->count,(PciPort+i)->bus,(PciPort+i)->dev,(PciPort+i)->fun);
						i++;
					}
					token = strtok(NULL, DELIMITERS_TYPE3);
				}
				if(find == 2)
					num++;
			}
		}
	}
	
	fclose(fptr);
	return i;
}

void PciTopologyOutput(char PortDefine[BUFFER_SIZE], char PortItem[][BUFFER_SIZE], char DeviceItem[][BUFFER_SIZE], int PortItemNum, int DeviceItemNum, char *DisplayName)
{
	FILE *fptr;
	char buffer[BUFFER_SIZE], *token, name[32];
	int counter, num, i=0, j, k, kk=0, CapID, offset, hi, lo, value, value_read, find, DeviceBus;
	u32 address, temp;
	
	char path[128]={};
	strcat(path,PCI_PATH);
	strcat(path,REGISTER_CONFIG);
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return;
	}
	
	if(PortDefine[0]!='\0')
	{
		path[0]='\0';
		strcat(path,PLATFORM_PATH);
		strcat(path,ROOTPORT_CONFIG);
		num=PciPortDefine(PortDefine, path);
		
		while(i<num)
		{
			// For root port
			printf("================================================================================\n");
			printf("%s%d       |",DisplayName,(PciPort+i)->count);
			for(;i<=num;i++)
			{
				printf("%-5s|",(PciPort+i)->name);
				if((i+1) == num || (PciPort+i+1)->count > (PciPort+i)->count)
					break;
			}
			printf("\n--------------------------------------------------------------------------------\n");
			
			for(j=0;j<PortItemNum;j++)
			{
				printf("%-11s%s",PortItem[j],"|");
				for(k=kk;k<=i;k++)
				{
					fseek(fptr, 0, SEEK_SET);

					while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
					{
						token = strtok(buffer, DELIMITERS_TYPE1);
						
						if(strchr(token, '#') == 0)
						{
							counter = 0;
							find = 1;
							while(token != NULL)
							{
								if(counter == 0)					// item name
								{
									if(strcmp(token,PortItem[j]))
										break;
									counter = 1;
								}
								else if(counter == 1)				// option
								{
									name[0]='\0';
									strncpy(name, token, sizeof(name));
									counter = 2;
								}
								else if(counter == 2)				// CapID
								{
									CapID = strtol(token, NULL, 16);
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
									address = get_pci_memory_addr(PCI_Base_Addr, (PciPort+k)->bus, (PciPort+k)->dev, (PciPort+k)->fun, 0);
									temp=MEMRW(READ, address, 4, 0);
									if(temp != 0xffffffff && temp != 0)
									{
										if(CapID != -1)
										{
											address = CapIDFind(PCI_Base_Addr, (PciPort+k)->bus, (PciPort+k)->dev, (PciPort+k)->fun, CapID);
											if(address != 0)
											{
												temp=MEMRW(READ, (address+offset), 4, 0);
												value_read=bit_range(temp, hi, lo);
												if(value == value_read)
												{
													token = strtok(name, DELIMITERS_TYPE1);
													if(token != NULL)
														printf("%-5s|",token);
													else
														printf("%-5s|"," ");
													break;
												}
											}
										}
										else
										{
											temp=MEMRW(READ, (address+offset), 4, 0);
											value_read=bit_range(temp, hi, lo);
											if(value == -1)
												printf("%-5x|",value_read);
											else
											{
												if(value == value_read)
												{
													token = strtok(name, DELIMITERS_TYPE1);
													if(token != NULL)
														printf("%-5s|",token);
													else
														printf("%-5s|"," ");
													break;
												}
											}
										}
									}
									else
									{
										find = 0;
										printf("%-5s|"," ");
										break;
									}
									counter = 7;
								}
								token = strtok(NULL, DELIMITERS_TYPE2);
							}
							if(find == 0)
								break;
						}
					}
				}
				printf("\n");
			}
			printf("--------------------------------------------------------------------------------\n");
			printf("--------------------------------------------------------------------------------\n");

			// For device
			for(j=0;j<DeviceItemNum;j++)
			{
				printf("%-11s%s",DeviceItem[j],"|");
				for(k=kk;k<=i;k++)
				{
					fseek(fptr, 0, SEEK_SET);

					while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
					{
						token = strtok(buffer, DELIMITERS_TYPE1);
						
						if(strchr(token, '#') == 0)
						{
							counter = 0;
							find = 1;
							while(token != NULL)
							{
								if(counter == 0)					// item name
								{
									if(strcmp(token,DeviceItem[j]))
										break;
									counter = 1;
								}
								else if(counter == 1)				// option
								{
									name[0]='\0';
									strncpy(name, token, sizeof(name));
									counter = 2;
								}
								else if(counter == 2)				// CapID
								{
									CapID = strtol(token, NULL, 16);
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
									address = get_pci_memory_addr(PCI_Base_Addr, (PciPort+k)->bus, (PciPort+k)->dev, (PciPort+k)->fun, 0x18);
									temp=MEMRW(READ, address, 4, 0);
									DeviceBus=bit_range(temp, 15, 8);
									if(temp != 0xffffffff && temp != 0)
									{
										address = get_pci_memory_addr(PCI_Base_Addr, DeviceBus, 0, 0, 0);
										temp=MEMRW(READ, address, 4, 0);
										if(temp != 0xffffffff && temp != 0)
										{
											if(CapID != -1)
											{
												address = CapIDFind(PCI_Base_Addr, DeviceBus, 0, 0, CapID);
												if(address != 0)
												{
													temp=MEMRW(READ, (address+offset), 4, 0);
													value_read=bit_range(temp, hi, lo);
													if(value == value_read)
													{
														token = strtok(name, DELIMITERS_TYPE1);
														if(token != NULL)
															printf("%-5s|",token);
														else
															printf("%-5s|"," ");
														break;
													}
												}
											}
											else
											{
												temp=MEMRW(READ, (address+offset), 4, 0);
												value_read=bit_range(temp, hi, lo);
												if(value == -1)
													printf("%-5x|",value_read);
												else
												{
													if(value == value_read)
													{
														token = strtok(name, DELIMITERS_TYPE1);
														if(token != NULL)
															printf("%-5s|",token);
														else
															printf("%-5s|"," ");
														break;
													}
												}
											}
										}
										else
										{
											find = 0;
											printf("%-5s|"," ");
											break;
										}
									}
									else
									{
										find = 0;
										printf("%-5s|"," ");
										break;
									}
									counter = 7;
								}
								token = strtok(NULL, DELIMITERS_TYPE2);
							}
							if(find == 0)
								break;
						}
					}
				}
				printf("\n");
			}
			printf("================================================================================\n\n");
			kk=i+1;
			i++;
		}
	}
	
	fclose(fptr);
}

void PciList(int type)
{
	int i,j,k,l, HeaderType, find, x1,y1,z1,x2,y2,z2,x,y,z, num, DeviceStatus_i = 0, ScanBusCounter, BusDevicePresent[255];
	u32 address, address2, value;
	u32 PortCapLinkWidth, PortStatusLinkWidth, PortCapLinkSpeed, PortStatusLinkSpeed, PortClassCode, PortVID, PortDID;
	u32 DeviceCapLinkWidth, DeviceStatusLinkWidth, DeviceCapLinkSpeed, DeviceStatusLinkSpeed, DeviceClassCode, DeviceVID, DeviceDID;
	
	num=sizeof(DeviceStatus)/sizeof(DeviceStatus[0]);
	for(l=0;l<num;l++)
	{
		DeviceStatus[l].index = -1;
	}
	
	num=sizeof(Device)/sizeof(Device[0]);
	for(l=0;l<num;l++)
	{
		Device[l].VID = 0;
		Device[l].DID = 0;
	}
	
	for(l=0;l<=0xff;l++)
		BusDevicePresent[l] = FALSE;
	
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
					PortVID=bit_range(value, 15, 0);
					PortDID=bit_range(value, 31, 16);
					find = FALSE;
					if(debug > 1)
						printf("[Debug] (All) Bus:%x Dev:%x Fun:%x VID_DID:%x\n",i, j, k, value);
					address = get_pci_memory_addr(PCI_Base_Addr, i, j, k, 0xc);
					value=MEMRW(READ, address, 4, 0);
					HeaderType = bit_range(value, 17, 16);
					if(type == ALL)
					{
						value=MEMRW(READ, (address-0x4), 4, 0);
						PortClassCode=bit_range(value, 31, 8);
						PciName(PortVID, PortDID, PortClassCode);
						
						address2 = CapIDFind(PCI_Base_Addr, i, j, k, 0x10);
						if(address2 != 0)
						{
							value=MEMRW(READ, (address2+0xc), 4, 0);
							PortCapLinkWidth=bit_range(value, 9, 4);
							PortCapLinkSpeed=bit_range(value, 3, 0);
							value=MEMRW(READ, (address2+0x10), 4, 0);
							PortStatusLinkWidth=bit_range(value, 25, 20);
							PortStatusLinkSpeed=bit_range(value, 19, 16);
							printf("%x:%x:%x %04x:%04x%s Cap:X%d Gen%d,Status:X%d Gen%d\n",i, j, k, PortVID, PortDID, PCI_NAME,PortCapLinkWidth,PortCapLinkSpeed,PortStatusLinkWidth,PortStatusLinkSpeed);
						}
						else
							printf("%x:%x:%x %04x:%04x%s\n",i, j, k, PortVID, PortDID, PCI_NAME);
					}
					else if(HeaderType == BRIDGE && BusDevicePresent[i] == FALSE)
					{
						// find root port
						address2 = CapIDFind(PCI_Base_Addr, i, j, k, 0x10);
						if(debug > 0)
							printf("[Debug] (Port) Bus:%x Dev:%x Fun:%x CapID 0x10 address:%x\n",i, j, k, address2);
						if(address2 != 0)
						{
							value=MEMRW(READ, (address-0x4), 4, 0);
							PortClassCode=bit_range(value, 31, 8);
							value=MEMRW(READ, (address2+0xc), 4, 0);
							PortCapLinkWidth=bit_range(value, 9, 4);
							PortCapLinkSpeed=bit_range(value, 3, 0);
							value=MEMRW(READ, (address2+0x10), 4, 0);
							PortStatusLinkWidth=bit_range(value, 25, 20);
							PortStatusLinkSpeed=bit_range(value, 19, 16);

							// find Decive
							address = get_pci_memory_addr(PCI_Base_Addr, i, j, k, 0x18);
							value=MEMRW(READ, address, 4, 0);
							x1=bit_range(value, 15, 8);
							x2=bit_range(value, 23, 16);
							y1=0;
							z1=0;
							y2=0x1f;
							z2=0x7;
							find = TRUE;
							
							PciName(PortVID, PortDID, PortClassCode);
							if(type == ROOTPORT)
								printf("%x:%x:%x %04x:%04x%s Cap:X%d Gen%d,Status:X%d Gen%d\n",i, j, k, PortVID, PortDID, PCI_NAME,PortCapLinkWidth,PortCapLinkSpeed,PortStatusLinkWidth,PortStatusLinkSpeed);
						}
					}
					
					if(find == TRUE)
					{
					ScanBusCounter=0;
					for(x=x1; x<=x2; x++)
					{
						ScanBusCounter++;
						BusDevicePresent[x] = TRUE;
					for(y=y1; y<=y2; y++)
					{
					for(z=z1; z<=z2; z++)
					{
						address = get_pci_memory_addr(PCI_Base_Addr, x, y, z, 0);
						value=MEMRW(READ, address, 4, 0);
						
						if(value != 0xffffffff && value != 0)
						{
							DeviceVID=bit_range(value, 15, 0);
							DeviceDID=bit_range(value, 31, 16);
							if(debug > 0)
								printf("[Debug] (Device) Bus:%x Dev:%x Fun:%x VID_DID:%x\n",x, y, z, value);
							address2 = CapIDFind(PCI_Base_Addr, x, y, z, 0x10);
							if(address2 != 0)
							{
								address = get_pci_memory_addr(PCI_Base_Addr, x, y, z, 0x8);
								value=MEMRW(READ, address, 4, 0);
								DeviceClassCode=bit_range(value, 31, 8);
								value=MEMRW(READ, (address2+0xc), 4, 0);
								DeviceCapLinkWidth=bit_range(value, 9, 4);
								DeviceCapLinkSpeed=bit_range(value, 3, 0);
								value=MEMRW(READ, (address2+0x10), 4, 0);
								DeviceStatusLinkWidth=bit_range(value, 25, 20);
								DeviceStatusLinkSpeed=bit_range(value, 19, 16);
								PciName(DeviceVID, DeviceDID, DeviceClassCode);
								if(type == ROOTPORT)
								{
									for(l=0;l<ScanBusCounter;l++)
										printf("%s"," ");
									printf("-> %x:%x:%x %04x:%04x%s Cap:X%d Gen%d,Status:X%d Gen%d\n",x, y, z, DeviceVID, DeviceDID, PCI_NAME,DeviceCapLinkWidth,DeviceCapLinkSpeed,DeviceStatusLinkWidth,DeviceStatusLinkSpeed);
								}
								else if(type == COMPARE)
								{
									DeviceStatus[DeviceStatus_i].bus=x;
									DeviceStatus[DeviceStatus_i].dev=y;
									DeviceStatus[DeviceStatus_i].fun=z;
									DeviceStatus[DeviceStatus_i].CapLinkWidth=DeviceCapLinkWidth;
									DeviceStatus[DeviceStatus_i].StatusLinkWidth=DeviceStatusLinkWidth;
									DeviceStatus[DeviceStatus_i].CapLinkSpeed=DeviceCapLinkSpeed;
									DeviceStatus[DeviceStatus_i].StatusLinkSpeed=DeviceStatusLinkSpeed;
									DeviceStatus[DeviceStatus_i].PortCapLinkWidth=PortCapLinkWidth;
									DeviceStatus[DeviceStatus_i].PortCapLinkSpeed=PortCapLinkSpeed;
									DeviceStatus[DeviceStatus_i].index=l;
									
									if(z == 0)
									{
										for(l=0;l<num && z==0;l++)
										{
											if(Device[l].VID == DeviceVID && Device[l].DID == DeviceDID)
											{
												Device[l].ScanCount=ScanBusCounter;
												DeviceStatus[DeviceStatus_i].index=l;
												break;
											}
											else if(Device[l].VID == 0 && Device[l].DID == 0)
											{
												strncpy( Device[l].name, PCI_NAME, sizeof(Device[l].name) );
												Device[l].VID = DeviceVID;
												Device[l].DID = DeviceDID;
												Device[l].ScanCount = 1;
												DeviceStatus[DeviceStatus_i].index=l;
												break;
											}
										}
									}
									DeviceStatus_i++;
								}
							}
						}
						//else if(z==0)
						//	break;
					}
					}
					}
					}
				}
				//else if(k==0)
				//	break;
			}
		}
	}
	if(type == COMPARE)
		DeviceCountCompare();
}

void DeviceCountCompare()
{
	int i,j,num,num1, FailCount;
	
	num1=sizeof(DeviceStatus)/sizeof(DeviceStatus[0]);
	num=sizeof(Device)/sizeof(Device[0]);
	for(i=0;i<num;i++)
	{
		if(Device[i].VID == 0 && Device[i].DID == 0)
			break;
		else
		{
			printf("Type the number of%s:",Device[i].name);
			scanf("%d", &Device[i].InputCount);
		}
	}
	printf("\n================================================================================\n");
	
	for(i=0;i<num;i++)
	{
		if(Device[i].VID == 0 && Device[i].DID == 0)
			break;
		else
		{
			if(Device[i].InputCount == Device[i].ScanCount)
				printf("%s:PASS ",Device[i].name);
			else
				printf("%s:FAIL ",Device[i].name);
			printf("(Total:%d->Real:%d)\n", Device[i].InputCount, Device[i].ScanCount);
			
			for(j=0;j<num1;j++)
			{
				if(DeviceStatus[j].index == i)
				{
					printf("  %x:%x:%x Cap:X%d Gen%d,Status:X%d Gen%d: ",DeviceStatus[j].bus,DeviceStatus[j].dev,DeviceStatus[j].fun,DeviceStatus[j].CapLinkWidth,DeviceStatus[j].CapLinkSpeed,DeviceStatus[j].StatusLinkWidth,DeviceStatus[j].StatusLinkSpeed);
					FailCount = 0;
					if(DeviceStatus[j].CapLinkWidth <= DeviceStatus[j].PortCapLinkWidth)
					{
						if(DeviceStatus[j].StatusLinkWidth < DeviceStatus[j].CapLinkWidth)
						{
							FailCount++;
							printf("[Link Width Fail]");
						}
					}
					else
					{
						if(DeviceStatus[j].StatusLinkWidth < DeviceStatus[j].PortCapLinkWidth)
						{
							FailCount++;
							printf("[Link Width Fail]");
						}
						else if(DeviceStatus[j].StatusLinkWidth < DeviceStatus[j].CapLinkWidth)
						{
							FailCount++;
							printf("[Link Width is limited Root Port(Cap:X%d)]", DeviceStatus[j].PortCapLinkWidth);
						}
					}
					
					if(DeviceStatus[j].CapLinkSpeed <= DeviceStatus[j].PortCapLinkSpeed)
					{
						if(DeviceStatus[j].StatusLinkSpeed < DeviceStatus[j].CapLinkSpeed)
						{
							FailCount++;
							printf("[Link Speed Fail]");
						}
					}
					else
					{
						if(DeviceStatus[j].StatusLinkSpeed < DeviceStatus[j].PortCapLinkSpeed)
						{
							FailCount++;
							printf("[Link Speed Fail]");
						}
						else if(DeviceStatus[j].StatusLinkSpeed < DeviceStatus[j].CapLinkSpeed)
						{
							FailCount++;
							printf("[Link Speed is limited Root Port(Cap:Gen%d)]", DeviceStatus[j].PortCapLinkSpeed);
						}
					}
					if(FailCount == 0)
						printf("PASS");
					printf("\n");
				}
				else if(DeviceStatus[j].index == -1)
					break;
			}
		}
	}
}

void PciName(u32 VID, u32 DID, u32 ClassCode)
{
	FILE *fptr;
	int counter = 0;
	u32 VendorID, DeviceID, BaseClass;
	char buffer[BUFFER_SIZE];
	char *token;
	
	char path[128]={};
	strcat(path,PCI_PATH);
	strcat(path,PCINAME_CONFIG);
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return;
	}
	fseek(fptr, 0, SEEK_SET);
	
	while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
	{
		token = strtok(buffer, DELIMITERS_TYPE2);
		counter = 0;
		
		while(token != NULL)
		{
			if(counter == 0)					// VID
			{
				VendorID = strtol(token, NULL, 16);
				if(VendorID != VID)
					break;
				counter = 1;
			}
			else if(counter == 1)				// DID
			{
				DeviceID = strtol(token, NULL, 16);
				if(DeviceID != DID)
					break;
				counter = 2;
			}
			else if(counter == 2)				// PCI Name
			{
				strncpy( PCI_NAME, token, sizeof(PCI_NAME) );
				return;
			}
			token = strtok(NULL, DELIMITERS_TYPE2);
		}
	}
	fclose(fptr);
	
	BaseClass=bit_range(ClassCode, 23, 16);
	switch(BaseClass)
	{
		case 0:
			strncpy( PCI_NAME, " Device was built before Class Code definitions were finalized", sizeof(PCI_NAME) );
			break;
		case 1:
			strncpy( PCI_NAME, " Mass storage controller", sizeof(PCI_NAME) );
			break;
		case 2:
			strncpy( PCI_NAME, " Network controller", sizeof(PCI_NAME) );
			break;
		case 3:
			strncpy( PCI_NAME, " Display controller", sizeof(PCI_NAME) );
			break;
		case 4:
			strncpy( PCI_NAME, " Multimedia device", sizeof(PCI_NAME) );
			break;
		case 5:
			strncpy( PCI_NAME, " Memory controller", sizeof(PCI_NAME) );
			break;
		case 6:
			strncpy( PCI_NAME, " Bridge device", sizeof(PCI_NAME) );
			break;
		case 7:
			strncpy( PCI_NAME, " Simple communication controllers d", sizeof(PCI_NAME) );
			break;
		case 8:
			strncpy( PCI_NAME, " Base system peripherals", sizeof(PCI_NAME) );
			break;
		case 9:
			strncpy( PCI_NAME, " Input devices", sizeof(PCI_NAME) );
			break;
		case 10:
			strncpy( PCI_NAME, " Docking stations", sizeof(PCI_NAME) );
			break;
		case 11:
			strncpy( PCI_NAME, " Processors", sizeof(PCI_NAME) );
			break;
		case 12:
			strncpy( PCI_NAME, " Serial bus controllers", sizeof(PCI_NAME) );
			break;
		case 13:
			strncpy( PCI_NAME, " Wireless controller", sizeof(PCI_NAME) );
			break;
		case 14:
			strncpy( PCI_NAME, " Intelligent I/O controllers", sizeof(PCI_NAME) );
			break;
		case 15:
			strncpy( PCI_NAME, " Satellite communication controllers", sizeof(PCI_NAME) );
			break;
		case 16:
			strncpy( PCI_NAME, " Encryption/Decryption controllers", sizeof(PCI_NAME) );
			break;
		case 17:
			strncpy( PCI_NAME, " Data acquisition and signal processing controllers", sizeof(PCI_NAME) );
			break;
		default:
			strncpy( PCI_NAME, " Reserved", sizeof(PCI_NAME) );
	}
}

u32 get_pci_memory_addr(u64 PCI_Base_Addr, int bus, int dev, int fun, int offset)
{
	u32 addr;
	addr=PCI_Base_Addr | (bus << 20) | (dev << 15) | (fun << 12) | offset;
	return addr;
}

u32 get_pci_mmio_addr(u64 PCI_Base_Addr, int bus, int dev, int fun, u32 bar, u32 offset)
{
	u32 addr;
	addr = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, bar);
	addr=MEMRW(READ, addr, 4, 0);
	addr = (addr & 0xfffffff0) + offset;
	return addr;
}

u32 bit_range(u32 value, int high, int low)
{
	u32 all=0xffffffff >> (31-high);
	value=(value&all) >> low;
	return value;
}

u32 set_bit(u32 read_value, int high, int low, u32 value)
{
	u32 temp = (bit_range(read_value, high, low)) << low;
	temp = read_value - temp;
	value = (temp | (value << low));
	return value;
}

u32 CapIDFind(u64 PCI_Base_Addr, int bus, int dev, int fun, u8 capid)
{
	u32 address, value, tcapid;
	
	address = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, 0x34);
	value=MEMRW(READ, address, 4, 0);
	value=bit_range(value, 7, 0);
	//printf("kimi AA \n");
      //  printf("value %x \n",value);
	while((value != 0)&&(value != 0xff) )
	{//printf("kimi2 \n");
         //printf("value %x \n",value);
		address = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, value);
		value=MEMRW(READ, address, 4, 0);
		tcapid=bit_range(value, 7, 0);
		if(tcapid == capid)
			return address;
		value=bit_range(value, 15, 8);
	}
	return 0;
}

u32 ExtendedCapIDFind(u64 PCI_Base_Addr, int bus, int dev, int fun, u32 capid)
{
	u32 address, value, tcapid, tcapid2;
	
	address = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, 0x100);
	value=MEMRW(READ, address, 4, 0);
	if(value == 0xffffffff)
		return 0;
	tcapid=bit_range(value, 15, 0);
	
	while(tcapid!=capid)
	{
		if(tcapid == 0)
			break;
		value=bit_range(value, 31, 20);   // Next Capability Offset
		if(value == 0 || value < 0x100)
			break;
		address = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, value);
		
		tcapid2 = tcapid;
		value=MEMRW(READ, address, 4, 0);
		tcapid=bit_range(value, 15, 0);
		
		// Workaround for Aspeed AST2300 (Next extended Capability Offset is infinite loop)
		if(tcapid == tcapid2)
			break;
	}
	
	if(tcapid == capid)
		return address;
	return 0;
}

void PciErrorOutput(u32 address, int ItemNum, char type[], int decode)
{
	FILE *fptr;
	char path[128]={}, buffer[BUFFER_SIZE], *token, name[128];
	int counter, i, offset, hi, lo, FailCounter=0, FindItem=FALSE;
	u32 value, addr, read_value, address2, PortCapLinkWidth;
	
	if(!strcmp(type,"Uncorrectable"))
		printf("Uncorrectable Error Status: ");
	else if(!strcmp(type,"Correctable"))
		printf("Correctable Error Status: ");
	
	if(decode == TRUE)
	{
		printf("\n+----------------------------------------------------------------+\n");
		printf("|%-5s|%-50s|%-7s|"," Bit"," Register Description"," Value");
		printf("\n+-----+--------------------------------------------------+-------+\n");
	}
	
	strcat(path,PCI_PATH);
	strcat(path,ERROR_REGISTER);

	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return 0;
	}
	
	for(i=0; i<ItemNum; i++)
	{
		fseek(fptr, 0, SEEK_SET);
		while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
		{
			token = strtok(buffer, DELIMITERS_TYPE2);
			if(strchr(token, '#') == 0)
			{
				counter = 0;
				while(token != NULL)
				{
					if(counter == 0)					// item name
					{
						if(strcmp(token,PciErrorItem[i].name))
							break;
						counter = 1;
					}
					else if(counter == 1)				// setting
					{
						strncpy(name, token, sizeof(name));
						counter = 2;
					}
					else if(counter == 2)				// type
					{
						if(strcmp(token,type))
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
						// Work around for PCI Lane Error Status
						if(!strcmp(type,"Lane"))
						{
							address2 = address & 0xfffff000;
							address2 = CapIDFind(address2, 0, 0, 0, 0x10);
							value=MEMRW(READ, (address2+0xc), 4, 0);
							PortCapLinkWidth=bit_range(value, 9, 4);
							
							if(lo >= PortCapLinkWidth)
								break;
						}
					
						value = strtol(token, NULL, 16);
						addr = address + offset;
						read_value=MEMRW(READ, addr, 4, 0);
						read_value=bit_range(read_value, hi, lo);
						
						if(read_value == value)
						{
							FindItem=TRUE;
							if(decode == TRUE)
							{
								//printf("%s,%s\n",PciErrorItem[i].name,name);
								printf("| %02d  |%-50s|   %x   |\n",lo,PciErrorItem[i].name,value);
							}
							else
							{
								if(value > 0)
								{
									FailCounter++;
									//if(FindItem-=FALSE)
									if(FailCounter == 1)
										printf("\n");
									printf(" -> %-50s: %s\n",PciErrorItem[i].name,name);
								}
							}
						}
						counter = 7;
					}
					token = strtok(NULL, DELIMITERS_TYPE2);
				}
			}
		}
	}
	fclose(fptr);
	if(decode == FALSE)
	{
		if(FailCounter == 0 && FindItem!=FALSE)
			printf("No error\n");
		else if(FindItem==FALSE)
			printf("No error\n");
	}
	else
		printf("+----------------------------------------------------------------+\n");
}

void PciErrorStatus(int decode, char type[])
{
	FILE *fptr;
	char path[128]={}, buffer[BUFFER_SIZE], *token;
	int ItemNum=0, counter, i,j,k;
	u32 address, value, VID, DID, ClassCode, address1, value1,address6;
	
	strcat(path,PCI_PATH);
	strcat(path,ERROR_ITEM);			// PCI Error status
	
	fptr = fopen(path, "r");			// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return 0;
	}
	fseek(fptr, 0, SEEK_SET);
	
	while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
	{
		token = strtok(buffer, DELIMITERS_TYPE2);
		if(strchr(token, '#') == 0)
		{
			counter = 0;
			while(token != NULL)
			{
				if(counter == 0 && !strcmp(type, token))	// type
				{
					strncpy(PciErrorItem[ItemNum].type, token, sizeof(PciErrorItem[ItemNum].type));
					counter = 1;
				}
				else if(counter == 1)				// item name
				{
					strncpy(PciErrorItem[ItemNum].name, token, sizeof(PciErrorItem[ItemNum].name));
					counter = 2;
				}
				else if(counter == 2)				// PCI Capability ID
				{
					PciErrorItem[ItemNum].CapID = strtol(token, NULL, 16);
					counter = 3;
				}
				else if(counter == 3)				// PCI Express Extended Capability ID
				{
					PciErrorItem[ItemNum].ExtCapID = strtol(token, NULL, 16);
					counter = 4;
					ItemNum++;
				}
				token = strtok(NULL, DELIMITERS_TYPE2);
			}
		}
	}
	fclose(fptr);
	
	for(i=StartBus; i<=EndBus; i++)
	{
		for(j=0; j<=0x1f; j++)
		{
			for(k=0; k<=0x7; k++)
			{
				address = get_pci_memory_addr(PCI_Base_Addr, i, j, k, 0);
				value=MEMRW(READ, address, 4, 0);
                                //address1 = get_pci_memory_addr(PCI_Base_Addr, i, j, k, 0x100); //Kimi++ 20170215 for Skyleak-SP hang up issue workaround 
				//value1=MEMRW(READ, address1, 4, 0);                            //Kimi++ 20170215 for Skyleak-SP hang up issue workaround. BDF did/vid = BDF offset 0x100~103 AER and Lane error check skip
                                address6 = CapIDFind(PCI_Base_Addr, i, j, k, 0x10);
                              /*  printf("address %x \n",address);
                                printf("value %x \n",value);
                                printf("address1 %x \n",address1);
                                printf("value1 %x \n",value1);*/
                                 
				if((value != 0xffffffff && value != 0)&&(address6 != 0))
				{
					VID=bit_range(value, 15, 0);
					DID=bit_range(value, 31, 16);
					value=MEMRW(READ, (address+8), 4, 0);
					ClassCode=bit_range(value, 31, 8);
					
					if(PciErrorItem[0].CapID != 0)
						address = CapIDFind(PCI_Base_Addr, i, j, k, PciErrorItem[0].CapID);
					else if(PciErrorItem[0].ExtCapID != 0)
						address = ExtendedCapIDFind(PCI_Base_Addr, i, j, k, PciErrorItem[0].ExtCapID);
					else
						address = 0;
					
					if(address != 0)			// find CapID
					{
						if(debug > 1)
						{
							if(PciErrorItem[0].CapID != 0)
								printf("[Debug] Bus:%x Dev:%x Fun:%x CapID:%x Addr:%x\n",i, j, k, PciErrorItem[0].CapID, address);
							else
								printf("[Debug] Bus:%x Dev:%x Fun:%x ExtCapID:%x Addr:%x\n",i, j, k, PciErrorItem[0].ExtCapID, address);
						}
						PciName(VID, DID, ClassCode);
						
						if(!strcmp("AER", type))
						{
							printf("================================================================================\n");
							printf("%x:%x:%x %04x:%04x%s\n",i, j, k, VID, DID, PCI_NAME);
							printf("--------------------------------------------------------------------------------\n");
							PciErrorOutput(address, ItemNum, "Uncorrectable", decode);
							printf("\n");
							//printf("--------------------------------------------------------------------------------\n");
							PciErrorOutput(address, ItemNum, "Correctable", decode);
							printf("================================================================================\n\n");
						}
						else
						{
							printf("%x:%x:%x %04x:%04x%s : ",i, j, k, VID, DID, PCI_NAME);
							PciErrorOutput(address, ItemNum, PciErrorItem[0].type, decode);
						}
					}
				}
                                 
			}
		}
	}
}

u32 GPIOSupport()
{
	FILE *fptr;
	char buffer[BUFFER_SIZE], *token;
	char path[128]={};
	int counter, bus, dev, fun, offset;
	u32 address=0;
	
	strcat(path,PLATFORM_PATH);
	strcat(path,SUPPORT_GPIO);
	fptr = fopen(path, "r");				// open file
	if(!fptr)
	{
		printf("\n[Error] Can not open %s\n", path);
		return 0;
	}
	fseek(fptr, 0, SEEK_SET);
	
	while(fgets(buffer, BUFFER_SIZE, fptr) != NULL)			// read string from file
	{
		counter = 0;
		token = strtok(buffer, DELIMITERS_TYPE1);
		if(strchr(token, '#') == 0)
		{
			while(token != NULL)
			{
				if(counter == 0)					// SB
				{
					if(strcmp(token,SB_PCI_DEFINE))
							break;
					counter = 1;
				}
				else if(counter == 1)				// bus
				{
					bus = strtol(token, NULL, 16);
					counter = 2;
				}
				else if(counter == 2)				// dev
				{
					dev = strtol(token, NULL, 16);
					counter = 3;
				}
				else if(counter == 3)				// fun
				{
					fun = strtol(token, NULL, 16);
					counter = 4;
				}
				else if(counter == 4)				// offset
				{
					offset = strtol(token, NULL, 16);
					counter = 5;
					address = get_pci_memory_addr(PCI_Base_Addr, bus, dev, fun, offset);
					address=MEMRW(READ, address, 4, 0);
					address=address & 0xfff0;
				}
				else if(counter == 5)				// GPIO DEFINE
				{
					strncpy( GPIO_DEFINE, token, sizeof(GPIO_DEFINE) );
					counter = 6;
				}
				token = strtok(NULL, DELIMITERS_TYPE1);
			}
		}
	}
	fclose(fptr);
	return address;
}
