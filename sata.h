#ifndef __SATA_H__
#define __SATA_H__

#define IDE_MODE			0
#define AHCI_MODE			1
#define RAID_MODE			2
#define UNKNOW_MODE			3

#define ADDRESS_TYPE		0
#define MMIO_TYPE			1
#define IO_TYPE				2

#define SATA_ITEM			"SATAItem.txt"
#define SATA_ERROR_ITEM		"SATAErrorItem.txt"
#define SATA_REGISTER		"SATARegister.txt"
#define INTEL_RST			"IntelRSTRegister.txt"
#define SATA_ONOFF			"SATAOnOff.txt"

#define NORMAL_TYPE			0
#define ERROR_TYPE			1

#define SATAON			0
#define SATAOFF			1

struct SATARootPort {
	char name[128];		// name
};

void SATATopology(int type, int decode);
void SATATopologyOutput(u32 PortBaseAddr, int PortCount, int SATAmode, char PortItem[][BUFFER_SIZE], int PortItemNum, int OutputType, int decode);
void SATAOnOff(int OnOff, int port);

#endif						/* __SATA_H__ */
