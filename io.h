#ifndef __IO_H__
#define __IO_H__

#define READ			0
#define WRITE			1
#define GPIO_REGISTER	"GPIORegister.txt"
#define DELIMITERS_TYPE1			"\n, "

#define GPIO			0
#define NATIVE			1
#define GPI				2
#define GPO				3
#define HIGH			4
#define LOW				5

struct GPIOStruct {
	char PortName[16];	// Port Name
	int GPIO_USE_SEL;	// GPIO / NATIVE
	int GP_IO_SEL;		// GPI / GPO
	int GP_LVL;			// High / Low
};

u32 iorw(int rw, u16 addr, u32 value, int length);
void GPIOTopology();

#endif						/* __IO_H__ */
