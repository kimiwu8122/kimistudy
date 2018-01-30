#ifndef __USB_H__
#define __USB_H__

#define EHCI_MODE			0
#define XHCI_MODE			1

#define ADDRESS_TYPE		0
#define MMIO_TYPE			1
#define IO_TYPE				2

#define SUPPORT_USB			"USBSupport.txt"
#define USB_EHCI_ITEM		"USBEHCIItem.txt"
#define USB_XHCI_ITEM		"USBXHCIItem.txt"
#define USB_ERROR_ITEM		"USBErrorItem.txt"
#define USB_REGISTER		"USBRegister.txt"
#define USB_ONOFF			"USBOnOff.txt"

#define NORMAL_TYPE			0
#define ERROR_TYPE			1

#define USBON			0
#define USBOFF			1

struct USBRootPort {
	char name[128];		// name
};

void USBTopology(int mode, int type, int decode);
void USBTopologyOutput(u32 PortBaseAddr, int PortCount, int mode, char PortItem[][BUFFER_SIZE], int PortItemNum, int OutputType, int decode, int OutputPortNum);
void USBOnOff(int OnOff, int port);

#endif						/* __USB_H__ */
