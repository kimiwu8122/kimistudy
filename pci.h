#ifndef __PCI_H__
#define __PCI_H__


#define NONE			0
#define BRIDGE			1
#define CARDBUS			2
#define DEVICE			3
#define All_PCI			4
#define BRIDGEDEVICE	5
#define VGA				0x030000
#define PCIBRIDGE		0x060401

#define ADDRESS			10
#define CAPID			11

#define RSDP_START_ADDR 0x000E0000     // RSDP start Physical Address
#define RSDP_END_OFFSET 0x00020000     // RSDP end Physical Address

#define PLATFORM_PATH				"./config/Platform/"
#define SUPPORT_NB					"SupportNorthBridge.txt"
#define SUPPORT_SB					"SupportSouthBridge.txt"
#define SUPPORT_GPIO				"GPIOSupport.txt"
#define PCI_PATH					"./config/PCI/"
#define ROOTPORT_CONFIG				"RootPort.txt"
#define DEVICE_CONFIG				"Device.txt"
#define REGISTER_CONFIG				"Register.txt"
#define PCINAME_CONFIG				"PCIName.txt"
//#define AER_ITEM					"AER.txt"
//#define AER_REGISTER				"AERRegister.txt"
#define ERROR_ITEM					"PciError.txt"
#define ERROR_REGISTER				"PciErrorRegister.txt"
#define NO_METHOD					0
#define SNB_Client_Method			1
#define SNB_Server_Method			2
//#define HSW_Client_Method			3
//#define SIX_Series_Method			4
//#define CaveCreek_Method			5

#define DELIMITERS_TYPE1			"\n, "
#define DELIMITERS_TYPE2			"\n,"
#define DELIMITERS_TYPE3			"\n,: "

#define ROOTPORT					0
#define COMPARE						1
#define ALL							2

#define BUFFER_SIZE		1024

char PCI_NAME [BUFFER_SIZE];
char NB_PCI_DEFINE[BUFFER_SIZE], SB_PCI_DEFINE[BUFFER_SIZE];
char GPIO_DEFINE [BUFFER_SIZE];

struct PciRootPort {
	char name[128];		// name
	int count;			// 
	int bus;			// bus
	int dev;			// dev
	int fun;			// fun
};

struct PciDeviceCount {
	char name[BUFFER_SIZE];		// name
	u32 VID;			// VID
	u32 DID;			// DID
	int ScanCount;		// scan count
	int InputCount;		// input count
};

struct PciDeviceStatus {
	int index;			// index of PciDeviceCount
	int bus;			// bus
	int dev;			// dev
	int fun;			// fun
	int CapLinkWidth;
	int StatusLinkWidth;
	int CapLinkSpeed;
	int StatusLinkSpeed;
	int PortCapLinkWidth;
	int PortCapLinkSpeed;
};

struct PciError {
	char name[128];		// name
	char type[128];		// type (Lane Error)
	int CapID;			// PCI Capability ID
	int ExtCapID;		// PCI Express Extended Capability ID
};

void GetPCIBaseAddress();
void CSRread(int bus, int dev, int fun, int offset);
int ChipsetSupport();
void PciTopology();
int PciPortDefine(char PortDefine[BUFFER_SIZE], char path[]);

void PciTopologyOutput(char PortDefine[BUFFER_SIZE], char PortItem[][BUFFER_SIZE], char DeviceItem[][BUFFER_SIZE], int PortItemNum, int DeviceItemNum, char *DisplayName);
void PciList(int type);
void DeviceCountCompare();

u32 get_pci_memory_addr(u64 PCI_Base_Addr, int bus, int dev, int fun, int offset);
u32 get_pci_mmio_addr(u64 PCI_Base_Addr, int bus, int dev, int fun, u32 bar, u32 offset);
u32 bit_range(u32 value, int high, int low);
u32 set_bit(u32 read_value, int high, int low, u32 value);
u32 CapIDFind(u64 PCI_Base_Addr, int bus, int dev, int fun, u8 CapID);
u32 ExtendedCapIDFind(u64 PCI_Base_Addr, int bus, int dev, int fun, u32 capid);
void PciName(u32 VID, u32 DID, u32 ClassCode);
void PciErrorOutput(u32 address, int ItemNum, char type[], int decode);
void PciErrorStatus(int decode, char type[]);
//void PciErrorOutput(u32 address, int ItemNum, char type[], int decode);
u32 GPIOSupport();

#endif						/* __PCI_H__ */