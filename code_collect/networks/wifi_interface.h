/************************************************
File name 	:wifi_interface.h
Description 	:API to setup or connect wifi
Company		:Sunnorth
Author		:yong.liu
Date		:Oct 14,2008
Version		:
Modify log 	:
****************************************************/

#ifndef _WIFI_INTERFACE_H_
#define	_WIFI_INTERFACE_H_

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define NET_FIFO "/var/network/net_fifo"
#define NET_PATH "/var/network/linux_netinfo"
#define SUCCESS 0
#define FAIL   -1

#define NET_DEBUG
#define SIG_NET 50
/*********************************************************************************/
#define BUFFERSIZE	4096
#define LEVEL1	64
#define LEVEL2	128
#define LEVEL3	192
#define LEVEL4  255
/*********************************************************************************/
struct NetStatus{
char IPAddr[16];
char SubNetMask[16];
char GateWay[16];
char DNSServer[2][16]; 
char MACAddr[20];
};
struct WiFi_Info{
char SSID[20];
int  password; 	/*indicate password or not,0 means no password,1 means password*/
int iSignal;
int iPassword_Format; /*0 mean no password,1 mean format is wep, 2 mean format is wpa, 3 mean format is wpa2*/
};
enum NetConfig{
	IPADDR = 0,
	SUBNETMASK,
	GATEWAY,
	DNSSERVER,
	MACADDR,
};
struct NetStatus Status;
struct WiFi_Info Scan[10];

typedef struct _N_Info_s
{
	struct NetStatus Status[2];
     	struct WiFi_Info Scan[10];
     	char szLink_SSID[20];
	char szLink_Password[20];
	int iStatus_Wire;
	int iStatus_Wireless;
	int iAction_Flag;       /*0 mean begin 1 mean end*/
	int iDhcp_Wire;
	int iDhcp_Wireless;
	int iSignal;
     	int  iSearchApNum;
	int iSearch_Flag; /*0 mean don't Search 1 mean Search wifi ap*/
	int iPassword_Format;
	int iConnect_Format;      /*0 mean don't connect;1 mean connect wire;2 mean connect wifi for password;
                                    3 mean connect wifi for auto*/
} N_Info_t;


#if 0
BOOL ConnectInternet();     //connect fail or success
int SearchWiFi();			//search usable internet(wifi or ethernet)
int ConnectWifi(char *pSSID);          //user select to use wifi SSID
int LoginUserName(char *pChar);			//login username
int LoginPassword(char *pChar);			//login password
int GetWifiSignal();					//current signal,have 4 levels
int SetWirelessMode(int iMode);			//set wireless mode or wired
int SetDHCP(int iEnable);               //enable DHCP or not
int GetNetStatus();						//get SSID/IP address/Net mask/Gateway address/DNS server/MAC address/
#endif

int GetNetStatus(const char *Name,struct NetStatus *Status);
int GetWiFiSignal(void);
int SearchWiFi(struct WiFi_Info *scan);								
int ConnectInternet(void);     //connect fail or success
int ConnectWifi(char *pSSID, char *pPassword, int key);

int ConnectWifiAuto(void);
void CloseWifi(void);

void CloseInternet(void);
int ReadNetInfo(N_Info_t *pNetInfo);
int SaveNetInfo(N_Info_t *pNetInfo);



int SearchWiFiDaemon(N_Info_t *pNetInfo);
int ConnectInternetDaemon(void);
int ConnectWifiDaemon(char *pSSID, char *pPassword);
int ConnectWifiAutoiDaemon(void);

void Clear_Action_Flag(void);

void DisplayNetInfo(N_Info_t *pNetInfo);
#endif
