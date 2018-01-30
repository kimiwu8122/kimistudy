#ifndef __SYSTEMAGENT_H__
#define __SYSTEMAGENT_H__

#define ADDRESS_TYPE		0
#define MMIO_TYPE			1
#define IO_TYPE				2

#define SYSTEM_AGENT_SUPPORT		"SystemAgentSupport.txt"
#define SYSTEM_AGENT_ITEM			"SystemAgentItem.txt"
#define SYSTEM_AGENT_REGISTER		"SystemAgentRegister.txt"

struct SARootPort {
	char name[128];		// name
};

int SystemAgentPortDefine(char SADefine[BUFFER_SIZE]);
void SystemAgent();
void SystemAgentOutput(char SAItem[][BUFFER_SIZE], int SAItemNum);


#endif						/* __SYSTEMAGENT_H__ */
