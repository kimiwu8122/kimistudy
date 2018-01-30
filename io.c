#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <errno.h>

#include "define.h"
#include "io.h"
#include "pci.h"

extern char GPIO_DEFINE [];

u32 iorw(int rw, u16 addr, u32 value, int length)
{
	int iopl_ret;
	if( iopl_ret = iopl(3) ) {
		fprintf(stderr, "\n[Error] iopl_ret: %d, error_no: %d\n",iopl_ret,errno);
		exit(1);
	}
	
	if(rw == WRITE && length == 4)
		outl(value,addr);
	if(rw == WRITE && length == 2)
		outw(value,addr);
	if(rw == WRITE && length == 1)
		outb(value,addr);
	else if(rw == READ && length == 4)
		return (inl(addr));
	else if(rw == READ && length == 2)
		return (inw(addr));
	else if(rw == READ && length == 1)
		return (inb(addr));
	return 0;
}

void GPIOTopology()
{
	FILE *fptr;
	u32 BaseAddress, offset, RegValue;
	char buffer[BUFFER_SIZE], *token, name[BUFFER_SIZE];
	char path[128]={};
	int counter, PortStartNum=9999, Type, hi, lo, value, i=0, j, value_read;
	struct GPIOStruct GPIOPort[128];
	
	BaseAddress = GPIOSupport();
	if(BaseAddress == 0)
		printf("The PCH is not supported.");
	if(debug >= 3)
		printf("[Debug] GPIO Base Address = %x\n", BaseAddress);
	
	strcat(path,PLATFORM_PATH);
	strcat(path,GPIO_REGISTER);
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
					if(strcmp(token,GPIO_DEFINE))
							break;
					counter = 99;
				}
				else if(counter == 99)				// Port Name
				{
					strncpy( name, token, sizeof(name) );
					counter = 1;
				}
				else if(counter == 1)				// Port Number
				{
					PortStartNum = strtol(token, NULL, 10);
					counter = 2;
				}
				else if(counter == 2)				
				{
					counter = 3;
				}
				else if(counter == 3)				// fun
				{
					if(!strcmp(token, "GPIO"))
						Type = GPIO;
					else if(!strcmp(token, "Native"))
						Type = NATIVE;
					else if(!strcmp(token, "GPI"))
						Type = GPI;
					else if(!strcmp(token, "GPO"))
						Type = GPO;
					else if(!strcmp(token, "High"))
						Type = HIGH;
					else if(!strcmp(token, "Low"))
						Type = LOW;
					counter = 4;
				}
				else if(counter == 4)				// offset
				{
					offset = strtol(token, NULL, 16);
					counter = 5;
				}
				else if(counter == 5)				// high bit
				{
					hi = strtol(token, NULL, 10);
					counter = 6;
				}
				else if(counter == 6)				// low bit
				{
					lo = strtol(token, NULL, 10);
					counter = 7;
				}
				else if(counter == 7)				// value
				{
					value = strtol(token, NULL, 16);
					RegValue=iorw(READ, (BaseAddress + offset), 0, 4);
					if(debug >= 3)
						printf("[Debug] GPIO Register Value = %x in address %x\n", RegValue, (BaseAddress + offset));
					
					for(i=lo; i<=hi; i++)
					{
						value_read=bit_range(RegValue, i, i);
						if(value == value_read)
						{
							strncpy( GPIOPort[PortStartNum+i].PortName, name, sizeof(GPIOPort[PortStartNum+i].PortName) );
							if(Type == GPIO || Type == NATIVE)
								GPIOPort[PortStartNum+i].GPIO_USE_SEL=Type;
							else if(Type == GPI || Type == GPO)
								GPIOPort[PortStartNum+i].GP_IO_SEL=Type;
							else if(Type == HIGH || Type == LOW)
								GPIOPort[PortStartNum+i].GP_LVL=Type;
						}
					}
					counter = 8;
				}
				token = strtok(NULL, DELIMITERS_TYPE1);
			}
		}
		if(Type == LOW && counter == 8)
		{
			for(j=PortStartNum; j<(PortStartNum+i); j++)
			{
				if(GPIOPort[j].GPIO_USE_SEL == NATIVE)
					printf("%s %-3d: %s\n", GPIOPort[j].PortName, j, "Native");
				else if(GPIOPort[j].GP_IO_SEL == GPI)
				{
					if(GPIOPort[j].GP_LVL == HIGH)
						printf("%s %-3d: %s\n", GPIOPort[j].PortName, j, "GPI High");
					else if(GPIOPort[j].GP_LVL == LOW)
						printf("%s %-3d: %s\n", GPIOPort[j].PortName, j, "GPI Low");
				}
				else if(GPIOPort[j].GP_IO_SEL == GPO)
				{
					if(GPIOPort[j].GP_LVL == HIGH)
						printf("%s %-3d: %s\n", GPIOPort[j].PortName, j, "GPO High");
					else if(GPIOPort[j].GP_LVL == LOW)
						printf("%s %-3d: %s\n", GPIOPort[j].PortName, j, "GPO Low");
				}
			}
		}
	}
	fclose(fptr);
}
