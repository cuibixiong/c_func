#include "wifi_interface.h"
#include "common_flist.h"
#include <errno.h>

#define NET_PATH "/var/network/linux_netinfo"
#define NET_FIFO "/var/network/net_fifo"
#if 0

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>


/*********************************************************************************/
#define BUFFERSIZE	4096
#define LEVEL1	64
#define LEVEL2	128
#define LEVEL3	192
#define LEVEL4  255
#define SUCCESS 0
#define FAIL    -1
#define DEBUG_WIFI
#ifdef DEBUG_WIFI
//#define MODESTRDEBUG_WIFI
#endif
/*********************************************************************************/
struct NetStatus{
char IPAddr[16];
char SubNetMask[16];
char GateWay[16];
char DNSServer[16];
char MACAddr[20];
};
struct WiFi{
char SSID[20];
int  password; 	/*indicate password or not,0 means no password,1 means password*/
};
enum NetConfig{
	IPADDR = 0,
	SUBNETMASK,
	GATEWAY,
	DNSSERVER,
	MACADDR,
};
int GetNetStatus(const char *Name,struct NetStatus *Status);
int GetWiFiSignal(void);
int SearchWiFi(struct WiFi *scan);								
int ConnectInternet(void);     //connect fail or success
int ConnectWifi(char *pSSID, char *pPassword);

#endif

/*********************************************************************************/
struct NetStatus Status;
struct WiFi_Info Scan[10];
char *NetMode[]={"inet addr:","Mask:","","","HWaddr "};
char *NetCommand[]={"/sbin/ifconfig ","cat /proc/net/wireless","iwlist eth1 scanning"};
/*********************************************************************************/
int GetModeStr(const char *buffer,const char *mode,char interval,const char *match);
int GetMatchStr(char *buffer,const char *interval,int num);
int GetWiFiStr(char *buffer,struct WiFi_Info *scan);
/*********************************************************************************/


#if 0
int main (int argc,char *argv[])
{ 
	struct NetStatus Status;
	GetNetStatus("eth0",&Status);
	#ifdef DEBUG_WIFI
	printf("ipaddr = %s\n",Status.IPAddr);
	printf("subnetmask = %s\n",Status.SubNetMask);
	printf("gateway = %s\n",Status.GateWay);
	printf("DNSServer = %s\n",Status.DNSServer);
	printf("MACAddr = %s\n",Status.MACAddr);	
	#endif
	struct WiFi *Scan=malloc(10*sizeof(struct WiFi));
	if(Scan == NULL)
		return 0;
	struct WiFi *p = Scan;
	SearchWiFi(p);
	free(p);
	int ret = 0;
	ret = GetWiFiSignal();
 	#ifdef DEBUG_WIFI
	printf("wifi signal level is %d\n",ret);
	#endif
		
# if 0	
	#ifdef TESTSEARCHWIFI
	struct WiFi *Scan=malloc(10*sizeof(struct WiFi));
	if(Scan == NULL)
		return 0;
	struct WiFi *p = Scan;
	int wfd = open("wifi.txt",O_RDONLY);
	char *temp[4096];
	int ret;
	read(wfd,temp,4096);
	ret = GetWiFiStr(temp,Scan);
	#ifdef DEBUG_WIFI
	int i;
	printf("ret = %d\n",ret);
	for(i = 0,p = Scan; i < ret; i++,p++)
	{
			printf("SSID[%d] = %s,password = %d\n",i,p->SSID,p->password);
	}
	#endif	
	free(Scan);
	#endif
#endif
} 

#endif
/*read the buffer,to find the string that match the mode before the interval*/
int GetModeStr(const char *buffer,const char *mode,char interval,const char *match)
{
	if((buffer == NULL)||(mode == NULL)||match == NULL)
		return -1;
	int str_len = strlen(buffer);
	int mode_len = strlen(mode);
	int i;
	char *begin = NULL,*end = NULL,*cur = NULL,*p = NULL;
	if((str_len == 0)||(mode_len == 0)) /*no need to do the followings*/
		return -1;
	begin = strstr(buffer,mode);
	if(begin == NULL)
		return -1;
	begin += strlen(mode);
	#ifdef MODESTRDEBUG_WIFI
	printf("begin string is %s\n",begin);
	#endif
	end = strchr(begin,interval);
	if(end == NULL)
		return -1;
	#ifdef MODESTRDEBUG_WIFI
	printf("end string is %s\n",end);
	#endif
	p = match;
	for(cur = begin; cur < end; *p++ = *cur++);
	#ifdef MODESTRDEBUG_WIFI
	printf("match string is %s\n",match);
	#endif
	return 1;
	
	
}

//Deal with the str to match only '0-9' and '.' 
int DealMatchStr(const char *Source,const char *Dst)
{
	if((Dst == NULL)||(Source == NULL))
		return -1;
	char *p = Source,*q = Dst;
	int i=0;
	while(p != NULL)
	{
		if( (*p == 46) || ( (*p>=48) && (*p<=58) ) || ((*p>=65)&&(*p<=70)))
		{
			*q++ = *p++;
			i++;
		}else{
			break;	
		}
	}	
	*q = '\0';	
	return i;
}

int GetMatchStr(char *buffer,const char *interval,int num)
{
 	if(buffer == NULL)
		return -1;
	char *p = NULL;
	int i = 0;
	p = strtok(buffer,interval); //·Ö½â×Ö·û´®
	while((p = strtok(NULL,interval)) != NULL)
	{
		if(strlen(p) > 0)
			i++;
		if(i == num)
		{
			#ifdef DEBUG_WIFI
			printf("when i == num,p = %s\n",p);
			#endif
			return atoi(p);
		}
	}
	return -1;
}

int GetWiFiStr(char *buffer,struct WiFi_Info *scan)
{
	if((buffer == NULL) || (scan == NULL))
		return -1;
	char *p = NULL;
	char match[20],bak[20];
	int i = 0,j = 0,num = 0,flag = 0;
	p = strtok(buffer," ");
	struct WiFi_Info *temp = scan;
	while((p = strtok(NULL," ")) != NULL)
	{
		
		if(strlen(p) > 0){
			#ifdef DEBUG_WIFI
			printf("j = %d,p = %s\n",j,p);
			#endif
			memset(match,0,20);
			if(GetModeStr(p,"ESSID:\"",0x22,match) > 0)
			{
				flag = 1;
				strcpy(bak,match);		
				num = j;
			}
			if((flag == 1) && ((j-num) <= 3))
			{
				if(strstr(p,"Mode:Managed") != NULL)
				{
					flag = 2;
					num = j;
					printf("i = %d,bak = %s\n",i,bak);
					strcpy(temp->SSID,bak);
					memset(bak,0,20);
					
				}		
			}
			if((flag == 1)&&(j-num>3))
			{
				flag = 0;
			}
			if(flag == 2)
			{
				memset(match,0,20);
				if(GetModeStr(p,"key:",'\n',match) > 0){
					if(strstr(p,"off") != 0)
					{
						flag = 0;
						#ifdef DEBUG_WIFI
						printf("i = %d.pasword = %d",i,0);
						#endif
						temp->password = 0;
						i++;
						temp++;
					}else if(strstr(p,"on") != 0)
					{
						flag = 0;
						#ifdef DEBUG_WIFI
						printf("i = %d.pasword = %d",i,1);
						#endif
						temp->password = 1;
						i++;
						temp++;
					}		
				}
				
			}		
			j++;
		}
	
	}
	return i;
}
/*Name is ethX*/
int GetNetStatus(const char *Name,struct NetStatus *Status)
{
	if((Name == NULL)||(Status == NULL))
	{
		#ifdef DEBUG_WIFI
		printf("error in argument\n");
		#endif
		return 0;	
	}
	int fd[2];
        char command[100];
        char match[5][25];
	char Dst[5][25];
	char readbuffer[BUFFERSIZE];
        int result;
	int i;
        pid_t pid;
        if(pipe(fd)<0)
        {
		#ifdef DEBUG_WIFI
                printf("error in pipe!");
                #endif
		return 0;
        }
        pid=fork();
        if (pid < 0)
        {
		#ifdef DEBUG_WIFI
                printf("error in fork!");
                #endif
		return 0;
        }
        else if (pid == 0) /*child*/
        {
                strcpy(command,NetCommand[0]);
		strcat(command,Name);
		#ifdef DEBUG_WIFI
		printf("test1!\n");
                printf("command = %s\n",command);
                #endif
		close(1);
                dup(fd[1]);
                close(fd[0]);
                system(command);
		sleep(3);
        }
        else /*parent*/
        {
		
                close(fd[1]);
                memset(readbuffer,0,BUFFERSIZE);
                if((result = read(fd[0],readbuffer,BUFFERSIZE))>0)
		{
			#ifdef DEBUG_WIFI
			printf("test2!\n");
			printf("the command output is  : %s", readbuffer);
			#endif
			for(i = 0;i < 5; i++)
			{
				GetModeStr(readbuffer,NetMode[i],' ',match[i]);
				DealMatchStr(match[i],Dst[i]);
				#ifdef DEBUG_WIFI
				printf("match[%d] is %s\n",i,match[i]);
				#endif
			}
			strcpy(Status->IPAddr,Dst[IPADDR]);
			strcpy(Status->SubNetMask,Dst[SUBNETMASK]);
			strcpy(Status->GateWay,Dst[GATEWAY]);
			strcpy(Status->DNSServer,Dst[DNSSERVER]);
			strcpy(Status->MACAddr,Dst[MACADDR]);	
			DPRINTF("Status->MACAddr is %s,Dst[MACADDR] is %s\n",Status->MACAddr,Dst[MACADDR]);
			memset(readbuffer,0,BUFFERSIZE);
				
                }
		kill(pid, 9);
		waitpid(pid,NULL,0);
		DPRINTF("NetWork Stat is KO!\n");
             
        }
        return 1;
}

/*
 * in wireless.h the qulity is a __u8 struct,so the max value is 255,so we divide it by 4 
 * 0-63 level1
 * 64 - 127 level2
 * 128 - 191 level3
 * 192 - 255 level4
 * return value:
 * 0 means error occur
 * 1~4 means the related levels 
 */
int GetWiFiSignal(void)
{
	int fd[2];
        char command[100];
	char match[100];
//	char readbuffer[50];
	char *readbuffer = NULL;
        int result,i,quality,ret;
        pid_t pid;
        if(pipe(fd)<0)
        {
		#ifdef DEBUG_WIFI
                printf("error in pipe!");
                #endif
		return 0;
        }
        pid=fork();
        if (pid < 0)
        {
		#ifdef DEBUG_WIFI
                printf("error in fork!");
                #endif
		return 0;
        }
        else if (pid == 0) /*child*/
        {
                strcpy(command,NetCommand[1]);
		#ifdef DEBUG_WIFI
                printf("command = %s\n",command);
                #endif
		close(1);
                dup(fd[1]);
                close(fd[0]);
                system(command);
		sleep(2);
        }
        else /*parent*/
        {
		int state;
//		waitpid(pid,NULL,0);
		readbuffer = malloc(4096);
		if(readbuffer != NULL)
		{
			printf("mem is ok!/n");
		}
		else
		{
			DPRINTF("MEM IS ERROR!/n");
			exit(-1);

		}
                close(fd[1]);
                memset(readbuffer,0,BUFFERSIZE);
                if((result = read(fd[0],readbuffer,BUFFERSIZE))>0)
		{
			#ifdef DEBUG_WIFI
			printf("the command output is  : %s", readbuffer);
			#endif
			//char *temp = "eth1: 0001    0   105   165  0 0   0    1      2        0";
			/*wireless is default eth1*/
  			/*eth1: 0001    0   211   165        0      0      0      1      2    0*/
			ret = GetModeStr(readbuffer,"eth1:",0x00,match);
			if(ret > 0)
			{
				quality = GetMatchStr(match," ",2);
				if((quality >0) && (quality < LEVEL1))	
					ret = 1;
				else if((quality >= LEVEL1) && ( quality < LEVEL2))
					ret = 2;
				else if((quality >= LEVEL2) && (quality < LEVEL3))
					ret = 3;
				else if((quality >= LEVEL3) && (quality < LEVEL4))
					ret = 4;
				else if( quality == 0)
					ret = 0;
				else
					ret = -1;
			}
			free(readbuffer);
			readbuffer = NULL;
			DPRINTF("free is ok!\n");
			system("free");
			kill(pid,9);
			waitpid(pid,NULL,0);
			DPRINTF("kill child!\n");
			return ret;
				
                }
		

        }
	
}
/*
 *Search all the wifi list that can be used
 *the return value is the num of the wiif can be used
 */

int SearchWiFiDaemon(N_Info_t *pNetInfo)
{
	ReadNetInfo(&pNetInfo);
	pNetInfo->iSearch_Flag = 1;
	SaveNetInfo(pNetInfo);
}


int SearchWiFi(struct WiFi_Info *scan)								
{
	
#if 1
 	N_Info_t gNetInfo;
	if(scan == NULL)
	{
		#ifdef DEBUG_WIFI
		printf("error in argument\n");
		#endif
		return 0;	
	}
	int fd[2];
        char command[100];
        char match[10][20];
	char readbuffer[BUFFERSIZE];
        int result,i = 0,ret = 0;
        char *p,*q;
	pid_t pid;
        if(pipe(fd)<0)
        {
		#ifdef DEBUG_WIFI
                printf("error in pipe!");
                #endif
		return 0;
        }
        pid=fork();
        if (pid < 0)
        {
		#ifdef DEBUG_WIFI
                printf("error in fork!");
                #endif
		return 0;
        }
        else if (pid == 0) /*child*/
        {
		char End_Flag[] =";ifconfig";//niezhi add in tite
                strcpy(command,NetCommand[2]);
		strncat(command,End_Flag,strlen(End_Flag));//niezhi add in tite
		#ifdef DEBUG_WIFI
                printf("command = %s\n",command);
                #endif
		close(1);
                dup(fd[1]);
                close(fd[0]);
                system(command);
		sleep(10);
        }
        else /*parent*/
        {

                close(fd[1]);
                memset(readbuffer,0,BUFFERSIZE);
		while((result = read(fd[0],readbuffer,BUFFERSIZE))>0)
		{
//////////////////////////////////////////////////////////////////////
			//niezhi add in tite
			if(strstr(readbuffer,"Scan completed"))
			{
				DPRINTF("wifi is ko!\n");
				ret = GetWiFiStr(readbuffer,scan);
			}
			else
			{
				ret = 0;
			}
////////////////////////////////////////////////////////////////
			kill(pid,9);
			waitpid(pid,NULL,0);
			DPRINTF("Search child is killed!ret:%d\n",ret);
			return ret;
                }
        }
#endif	
}

int ConnectInternetDaemon(void)
{
 	N_Info_t gNetInfo;
	ReadNetInfo(&gNetInfo);
	gNetInfo.iConnect_Format = 1;
	SaveNetInfo(&gNetInfo);

}


int ConnectInternet(void)
{
	int ret;
	system("killall udhcpc");
	system("ifconfig eth0 down");
	system("ifconfig eth0 up");
	ret = system("udhcpc -i eth0 -n");	
	if(ret == 0)
	{
		return SUCCESS;	
	}
	else
	{
		return FAIL;	
	}
/*
	int fd;
	int len;
	char wBuffer[1];
	wBuffer[0] = 0;
//
	if(mkfifo(NET_FIFO,0666) < 0)
	{
		if(errno != EEXIST)
			exit(-1);
	}
//
	fd = open(NET_FIFO,O_WRONLY);
	write(fd,wBuffer,1);
	close(fd);

*/	
}

void CloseInternet(void)
{
	system("killall udhcpc");
	system("ifconfig eth0 down");

} 

int ConnectWifiDaemon(char *pSSID, char *pPassword)
{
	
 	N_Info_t gNetInfo;
	ReadNetInfo(&gNetInfo);
	memcpy(gNetInfo.szLink_SSID,pSSID,strlen(pSSID));
	memcpy(gNetInfo.szLink_Password,pPassword,strlen(pPassword));
	gNetInfo.iConnect_Format = 2;
	SaveNetInfo(&gNetInfo);
}


int ConnectWifi(char *pSSID, char *pPassword, int key)
{
/*
	int fd;
	int len;
	char wBuffer[1];
	wBuffer[0] = 1;
//
	if(mkfifo(NET_FIFO,0666) < 0)
	{
		if(errno != EEXIST)
			exit(-1);
	}
//
	fd = open(NET_FIFO,O_WRONLY);
	write(fd,wBuffer,1);
	len = strlen(pSSID);
	wBuffer[0] = len;
	write(fd,wBuffer,1);
	write(fd,pSSID,len);
	len = strlen(pPassword);
	wBuffer[0] = len;
	write(fd,wBuffer,1);
	write(fd,pPassword,len);

	close(fd);
*/
	int ret;
	int i;
	int keynum;
	char cmd_password[60] = "iwconfig eth1 key ";
	char cmd_every_password[60];
	char cmd_id[60] = "iwconfig eth1 essid ";
	DPRINTF("wifi is ok!\n");
	DPRINTF("PASSWORD:%s\n",pPassword);
	if(*pPassword == '\0')
	{
		keynum = 0;
	}
	else
	{
		keynum = 1;
	}
	switch(keynum)
	{
		case 0:
		{
			CloseWifi();
			system("iwconfig eth1 key off");
			strncat(cmd_id,pSSID,30);
			ret = system(cmd_id);
			sleep(1);
			if(ret == 0)
			{
				ret = system("udhcpc -i eth1 -n");
				sleep(1);
				if(ret == 0)
				{
					return SUCCESS;
				}
				else
				{
					return FAIL;
				}
			}
		}			
		case 1:
		{
			printf("case 1");
			CloseWifi();
			strncat(cmd_id, pSSID, 30);
			strncat(cmd_password, pPassword, 30);
			system(cmd_password);
			system("iwconfig eth1 key on");
			ret = system(cmd_id);
			sleep(1);
			if(ret != 0)
			{
				return FAIL;	
			} 
			ret = system("udhcpc -i eth1 -n");
			sleep(1);	
			if(ret == 0)
			{
				return SUCCESS;	

			}
		}
		case 2:
		{
			CloseWifi();
			memset(cmd_every_password,'\0',60);
			memcpy(cmd_every_password,cmd_password,60);
			strncat(cmd_every_password, " [2]",6);
			DPRINTF("cmd_every_password:%s\n",cmd_every_password);
			system(cmd_every_password);
			system("iwconfig eth1 key [2]");
			system("iwconfig eth1 key on");
			ret = system(cmd_id);
			sleep(1);
			if(ret != 0)
			{
				return FAIL;	
			} 
			ret = system("udhcpc -i eth1 -n");
			sleep(1);	
			if(ret == 0)
			{
				return SUCCESS;	

			}
		}
		case 3:
		{
			CloseWifi();
			memset(cmd_every_password,'\0',60);
			memcpy(cmd_every_password,cmd_password,60);
			strncat(cmd_every_password, " [3]",6);
			DPRINTF("cmd_every_password:%s\n",cmd_every_password);
			system(cmd_every_password);
			system("iwconfig eth1 key [3]");
			system("iwconfig eth1 key on");
			ret = system(cmd_id);
			sleep(1);
			if(ret != 0)
			{
				return FAIL;	
			} 
			ret = system("udhcpc -i eth1 -n");
			sleep(1);	
			if(ret == 0)
			{
				return SUCCESS;	

			}
		}
		case 4:
		{
			CloseWifi();
			memset(cmd_every_password,'\0',60);
			memcpy(cmd_every_password,cmd_password,60);
			strncat(cmd_every_password, " [4]",6);
			DPRINTF("cmd_every_password:%s\n",cmd_every_password);
			system(cmd_every_password);
			system("iwconfig eth1 key [4]");
			system("iwconfig eth1 key on");
			ret = system(cmd_id);
			sleep(1);
			if(ret != 0)
			{
				return FAIL;	
			} 
			ret = system("udhcpc -i eth1 -n");
			sleep(1);	
			if(ret == 0)
			{
				return SUCCESS;	

			}
		}
		break;
	}

	
#if 0
	if(*pPassword == '\0')
	{
		DPRINTF("THE PASSWORD IS NULL!\n");
		system("iwconfig eth1 key off");
		strncat(cmd_id, pSSID, 30);	
		ret = system(cmd_id);
		sleep(1);
		if(ret == 0)
		{
			
			ret = system("udhcpc -i eth1 -n");
			sleep(1);	
			if(ret == 0)
			{
				return SUCCESS;	
			}
			else
			{
				return FAIL;	
			}
		}
		else
		{
			return FAIL;	
		}
	}
	
	strncat(cmd_id, pSSID, 30);
	strncat(cmd_password, pPassword, 30);
	DPRINTF("id:%s\n", cmd_id);
	DPRINTF("password:%s\n", cmd_password);
//	system("ifconfig eth1 up");
//	system("iwconfig eth1 key off");
	system(cmd_password);
	system("iwconfig eth1 key on");
//	ret = system(cmd_id);
//	sleep(1);
//	if(ret != 0)
//	{
//		return FAIL;	
//	} 
	for(i = 0 ; i< 5; i++)
	{
		ret = system(cmd_id);
		sleep(1);
		if(ret == 0)
		{
			break;
		}
	}
	if(i == 5)
	{
		return FAIL;	
	} 	
	ret = system("udhcpc -i eth1 -n");
	sleep(1);	
	if(ret == 0)
	{
		return SUCCESS;	
	}
	else
	{
		return FAIL;	
	}
#endif
#if 0
	int ret;
	int i;
	char cmd_password[60] = "iwconfig eth1 key restricted [";
	char cmd_id[60] = "iwconfig eth1 essid ";
	char cmd_key[30] = "iwconfig eth1 key [";
	char *mode = NULL; 
	char p[3];
	p[0] = 48 + key;
	p[1] = 93;
	p[2] = 32; 
	DPRINTF("wifi is ok!\n");
	DPRINTF("PASSWORD:%s\n",pPassword);
	system("killall udhcpc");
	system("ifconfig eth1 down");
	system("ifconfig eth1 up");
	
	if(*pPassword == '\0')
	{
		DPRINTF("THE PASSWORD IS NULL!\n");
		system("iwconfig eth1 key off");
		strncat(cmd_id, pSSID, 30);	
		ret = system(cmd_id);
		sleep(1);
		if(ret == 0)
		{
			
			ret = system("udhcpc -i eth1 -n");
			sleep(1);	
			if(ret == 0)
			{
				return SUCCESS;	
			}
			else
			{
				return FAIL;	
			}
		}
		else
		{
			return FAIL;	
		}
	}
	
	strncat(cmd_id, pSSID, 30);
/*	mode = FindWifiMode();
	if(mode == NULL)
	{
		DPRINTF("DON'T HAVE KEY!\n");
		exit(-1);
	
	}
	DPRINTF("mode :%s\n",mode);*/
	strncat(cmd_password,p,3);
	strncat(cmd_key, p, 2);
	strncat(cmd_password, pPassword, 30);
	DPRINTF("id:%s\n", cmd_id);
	DPRINTF("password:%s\n", cmd_password);
	system("iwconfig eth1 key off");
	system(cmd_password);
	system(cmd_key);
	DPRINTF("key:%s\n", cmd_key);
	system("iwconfig eth1 key on");
	for(i = 0 ; i< 5; i++)
	{
		ret = system(cmd_id);
		sleep(1);
		if(ret == 0)
		{
			break;
		}
	}
	if(i == 5)
	{
		return FAIL;	
	} 	
	ret = system("udhcpc -i eth1 -n");
	sleep(1);	
	if(ret == 0)
	{
		return SUCCESS;	
	}
	else
	{
		return FAIL;	
	}
	

#endif	
}

int ConnectWifiAutoiDaemon(void)
{
	
 	N_Info_t gNetInfo;
	ReadNetInfo(&gNetInfo);
	gNetInfo.iConnect_Format = 3;
	SaveNetInfo(&gNetInfo);
}


int ConnectWifiAuto(void)
{
/*
	int fd;
	int len;
	char wBuffer[1];
	wBuffer[0] = 2;
//
	if(mkfifo(NET_FIFO,0666) < 0)
	{
		if(errno != EEXIST)
			exit(-1);
	}
//
	fd = open(NET_FIFO,O_WRONLY);
	write(fd,wBuffer,1);
	close(fd);


*/
	int ret;
	CloseWifi();
	ret = system("iwconfig eth1 essid any");
	if(ret == 0)
	{
		ret = system("udhcpc -i eth1 -n");
		sleep(1);
		if(ret == 0)
		{
			return SUCCESS;	
		}
		else
		{
			return FAIL;
		}
	}
	return FAIL;
	
}

void CloseWifi(void)
{
	int fd;
	system("killall udhcpc");
	system("iwpriv eth1 deauth");
	system("killall wpa_supplicant");
}

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

void DisplayNetInfo(N_Info_t *pNetInfo)
{
	int i;

	printf("***************NetStats***********************\n");
	printf("IPAddr:%s\n",pNetInfo->Status[0].IPAddr);
	printf("SubNetMask:%s\n",pNetInfo->Status[0].SubNetMask);
	printf("GateWay:%s\n",pNetInfo->Status[0].GateWay);
	printf("DNSServer 1:%s\n",pNetInfo->Status[0].DNSServer[1]);
	printf("DNSServer 2:%s\n",pNetInfo->Status[0].DNSServer[2]);
	printf("MacAddr:%s\n\n",pNetInfo->Status[0].MACAddr);

	printf("IPAddr:%s\n",pNetInfo->Status[1].IPAddr);
	printf("SubNetMask:%s\n",pNetInfo->Status[1].SubNetMask);
	printf("GateWay:%s\n",pNetInfo->Status[1].GateWay);
	printf("DNSServer 1:%s\n",pNetInfo->Status[1].DNSServer[1]);
	printf("DNSServer 2:%s\n",pNetInfo->Status[1].DNSServer[2]);
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

