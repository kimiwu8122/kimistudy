
#ifndef __DEFINE_H__
#define __DEFINE_H__

#include <stdint.h>

#ifndef DEFINE_TYPES
#define DEFINE_TYPES
typedef int8_t			s8;
typedef int16_t			s16;
typedef int32_t			s32;
typedef int64_t			s64;
typedef uint8_t			u8;
typedef uint16_t		u16;
typedef uint32_t		u32;
typedef uint64_t		u64;
#endif

#define PROGRAM_NAME 		"Pciscan"
#define VERSION                 "V1.3"
#define RELEASE_DATE		"02/17"
#define RELEASE_YEAR		"2017"
#define AUTHOR			"Sean Chou"

#define TRUE			1
#define FALSE			0
int debug;					// debug level


#endif						/* __DEFINE_H__ */
