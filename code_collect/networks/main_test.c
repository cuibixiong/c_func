#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "wifi_interface.h"


void DisplayNetInfo(N_Info_t *pNetInfo);
void Clear_Connect_Flag(void);
void Clear_Search_Flag(void);

/*
typedef struct _N_Info_s
{
     	char szIP[20];
     	char szNetMask[20];
     	char szSearch_SSID[10][20];
     	char szLink_SSID[20];
     	char szLink_Mac[20];
     	char szGetWay[20];
     	char szDNS[3][20];
	int bStatus_Wire;
	int bStatus_Wireless;
	int bDhcp_Wire;
	int bDhcp_Wireless;
     	int  iName;
     	int iSignal;
} N_Info_t;



#define NET_PATH "/var/network/linux_netinfo"
#define NET_FIFO "/var/network/net_fifo"
*/
int ReadNetInfo(N_Info_t *pNetInfo);

int ReadNetInfo(N_Info_t *pNetInfo)
{
	FILE *stream = NULL;
	int ret;

	stream = fopen(NET_PATH,"r");
	if(NULL == stream)
	{
		return -1;
	}
	
	ret = fread(pNetInfo,sizeof(N_Info_t),1,stream);

	fclose(stream);

	return 0;
}
int SaveNetInfo(N_Info_t *pNetInfo)
{
	FILE *stream = NULL;
	int ret;
	stream = fopen(NET_PATH ,"w");
	if(NULL == stream)
	{
		return -1;
	}
	fwrite(pNetInfo,sizeof(N_Info_t),1,stream);
	fclose(stream);
	return 0;
}



int main(void)
{
	int fd;
	int len;
	char wBuffer[1];
	char *szSSID = "linksys";
	char *szPassword = "nickzhi66";
	wBuffer[0] = 1;
	
 	N_Info_t gNetInfo;
	memset(&gNetInfo,'\0',sizeof(N_Info_t));
/*
	if(mkfifo(NET_FIFO,0666) < 0)
	{
		if(errno == EEXIST)
			printf("mk is error!\n");
		else
			exit(-1);
	}
	fd = open(NET_FIFO,O_WRONLY|O_NONBLOCK);
	write(fd,wBuffer,1);
	
	len = strlen(szSSID);
	printf("len:%d\n",len);
	wBuffer[0] = len;
	write(fd,wBuffer,1);
	write(fd,szSSID,len);
	len = strlen(szPassword);
	wBuffer[0] = len;
	write(fd,wBuffer,1);
	write(fd,szPassword,len);
	
	close(fd);
	return;
*/
/*
	while(1)
	{
		sleep(2);
		ReadNetInfo(&gNetInfo);
		gNetInfo.iSearch_Flag= 1;
		SaveNetInfo(&gNetInfo);
		DisplayNetInfo(&gNetInfo);

*/
		ReadNetInfo(&gNetInfo);
		strcpy(gNetInfo.szLink_SSID,"linksys");
		strcpy(gNetInfo.szLink_Password,"AA8465E6A4");
		gNetInfo.iConnect_Format = 2;
		SaveNetInfo(&gNetInfo);
		while(1)
		{
			ReadNetInfo(&gNetInfo);
			if((gNetInfo.iAction_Flag & 0x1) == 1)
			{
				if(gNetInfo.iStatus_Wireless == 1)
				{
					printf("connect is ok!\n");
				}
				else
				{
					printf("connect if fail!\n");
				}
				Clear_Connect_Flag();
			}
			else
			{
				DisplayNetInfo(&gNetInfo);
			}
		
			sleep(2);
			
		}
//	}	

}

void DisplayNetInfo(N_Info_t *pNetInfo)
{
	int i;

	printf("***************NetStats***********************\n");
	printf("IPAddr:%s\n",pNetInfo->Status[0].IPAddr);
	printf("SubNetMask:%s\n",pNetInfo->Status[0].SubNetMask);
	printf("GateWay:%s\n",pNetInfo->Status[0].GateWay);
	printf("DNSServer 1:%s\n",pNetInfo->Status[0].DNSServer[0]);
	printf("DNSServer 2:%s\n",pNetInfo->Status[0].DNSServer[1]);
	printf("MacAddr:%s\n\n",pNetInfo->Status[0].MACAddr);

	printf("IPAddr:%s\n",pNetInfo->Status[1].IPAddr);
	printf("SubNetMask:%s\n",pNetInfo->Status[1].SubNetMask);
	printf("GateWay:%s\n",pNetInfo->Status[1].GateWay);
	printf("DNSServer 1:%s\n",pNetInfo->Status[1].DNSServer[0]);
	printf("DNSServer 2:%s\n",pNetInfo->Status[1].DNSServer[1]);
	printf("MacAddr:%s\n",pNetInfo->Status[1].MACAddr);

	printf("*******************WiFi info Scan*************************\n");
	for(i = 0;strcmp(pNetInfo->Scan[i].SSID,"\0") != 0;i++)
	{
		printf("Scan[%i].SSID:%s,Key:%d,iSignal:%d\n",i,pNetInfo->Scan[i].SSID,pNetInfo->Scan[i].password,pNetInfo->Scan[i].iSignal);
	}

	printf("***************Connecting SSID and Key*********************************\n");
	printf("SSID:%s,KEY:%s\n",pNetInfo->szLink_SSID,pNetInfo->szLink_Password);

	printf("iStatus_Wire:	%d\n",pNetInfo->iStatus_Wire);
	printf("iStatus_Wireless:	%d\n",pNetInfo->iStatus_Wireless);
	printf("iAction_Flag:	%d\n",pNetInfo->iAction_Flag);
	printf("iDhcp_Wire:	%d\n",pNetInfo->iDhcp_Wire);
	printf("iDhcp_Wireless:	%d\n",pNetInfo->iDhcp_Wireless);
	printf("iSignal:	%d\n",pNetInfo->iSignal);
	printf("iSearchApNum:	%d\n",pNetInfo->iSearchApNum);
	printf("iSearch_Flag:	%d\n",pNetInfo->iSearch_Flag);
	printf("iPassword_Format:%d\n",pNetInfo->iPassword_Format);
	printf("iConnect_Format:%d\n",pNetInfo->iConnect_Format);
}
void Clear_Connect_Flag(void)
{
 	N_Info_t gNetInfo;
	ReadNetInfo(&gNetInfo);
	gNetInfo.iAction_Flag &= 0xfffffffe;
	SaveNetInfo(&gNetInfo);
}
void Clear_Search_Flag(void)
{
 	N_Info_t gNetInfo;
	ReadNetInfo(&gNetInfo);
	gNetInfo.iAction_Flag &= 0xfffffffb;
	SaveNetInfo(&gNetInfo);

}



