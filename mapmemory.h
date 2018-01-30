#ifndef __MAPMEMORY_H__
#define __MAPMEMORY_H__


#define READ			0
#define WRITE			1

u64 pagesize;

u8 *map_memory(int, u64, u64);
void unmap_memory(u8 *, u64);
u64 MEMRW(int, u64 , u64, u64);

#endif						/* __MAPMEMORY_H__ */
