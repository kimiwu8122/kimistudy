#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "define.h"
#include "mapmemory.h"

u8 *map_memory(int rw, u64 addr, u64 length)
{
	u64 offset;
	u8 *map_addr;
	pagesize = sysconf(_SC_PAGESIZE);
	int fd;
	offset = addr % pagesize;
	
	if(debug > 3)
		printf("[Debug] addr:%x, offset:%x\n", addr, offset);
	
	if(rw == READ)
	{
		fd = open("/dev/mem", O_RDONLY);
		if (fd < 0)
		{
			fprintf(stderr, "Can not open /dev/mem\n");
			return 0;
		}
		map_addr = mmap(NULL, (length + offset), PROT_READ, MAP_SHARED, fd, (addr - offset));
	}
	else
	{
		fd = open("/dev/mem", O_RDWR);
		if (fd < 0)
		{
			fprintf(stderr, "Can not open /dev/mem\n");
			return 0;
		}
		map_addr = mmap(NULL, (length + offset), PROT_WRITE, MAP_SHARED, fd, (addr - offset));
	}
	close(fd);
	if (map_addr == MAP_FAILED) 
		return 0;
	return (map_addr+offset);
}

/* unmap memory */
void unmap_memory(u8 * addr, u64 length)
{
	pagesize = sysconf(_SC_PAGESIZE);
	u64 offset = (u64)addr % pagesize;
	munmap(addr - offset, length + offset);
}

u64 MEMRW(int rw, u64 address, u64 length, u64 value)
{
	u8* memory_addr;

	if(!(memory_addr = map_memory(rw, address, length)))
	{
		printf("\nCan not map memory\n");
		unmap_memory(memory_addr, length);
		exit(1);
	}

	if(rw == READ)
	{
		if(length == 1)
			value= *((u8* )memory_addr);
		else if(length == 2)
			value= *((u16* )memory_addr);
		else if(length == 4)
			value= *((u32* )memory_addr);
		else if(length == 8)
			value= *((u64* )memory_addr);
	}
	else
	{
		if(length == 1)
			*(u8*) ( memory_addr )=value;
		else if(length == 2)
			*(u16*) ( memory_addr )=value;
		else if(length == 4)
			*(u32*) ( memory_addr )=value;
		else if(length == 8)
			*(u64*) ( memory_addr )=value;
	}
	
	unmap_memory(memory_addr, length);
	return value;
}
