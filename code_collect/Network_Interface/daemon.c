#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include "wifi_interface.h"
#include <errno.h>


N_Info_t gNetInfo;



char *szFileBuffer;


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
	p = (char*)match;
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
	char *p = NULL,*q = NULL;	
	p = (char*)Source;
	q = (char*)Dst;
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
	int signal;
	p = strtok(buffer," ");
	struct WiFi_Info *temp = scan;
	while((p = strtok(NULL," ")) != NULL)
	{
		
		if(strlen(p) > 0){
			#ifdef DEBUG_WIFI
			printf("j = %d,p = %s\n",j,p);
			#endif
			memset(match,'\0',20);
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
					#ifdef DEBUG_WIFI
					printf("i = %d,bak = %s\n",i,bak);
					#endif
					strcpy(temp->SSID,bak);
					memset(bak,'\0',20);
					
				}		
			}
			if((flag == 1)&&(j-num>3))
			{
				flag = 0;
			}
			if(flag == 2){
				memset(match,'\0',20);
				if(GetModeStr(p,"level=-",'\0',match) > 0){
					signal=atoi(match);
					if(signal < 40){
						temp->iSignal = 4;
					}
					else if((signal >39) && (signal < 60)){
						temp->iSignal = 3;
					}
					else if((signal >59) && (signal < 80)){
						temp->iSignal = 2;
					}
					else if((signal >79) && (signal < 100)){
						temp->iSignal = 1;
					}
					else{
						temp->iSignal = 0;	
					}
					flag = 3;
				}
			}	
			if(flag == 3)
			{
				memset(match,'\0',20);
				if(GetModeStr(p,"key:",'\n',match) > 0){
					if(strstr(p,"off") != 0)
					{
						#ifdef DEBUG_WIFI
						printf("i = %d.pasword = %d",i,0);
						#endif
						temp->password = 0;
					}else if(strstr(p,"on") != 0)
					{
						#ifdef DEBUG_WIFI
						printf("i = %d.pasword = %d",i,1);
						#endif
						temp->password = 1;
					}
					flag = 0;		
					i++;
					temp++;
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
                memset(readbuffer,'\0',BUFFERSIZE);
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
			GetDNSStr(Status);
			strcpy(Status->IPAddr,Dst[IPADDR]);
			strcpy(Status->SubNetMask,Dst[SUBNETMASK]);
			strcpy(Status->GateWay,Dst[GATEWAY]);
			strcpy(Status->MACAddr,Dst[MACADDR]);	
			memset(readbuffer,'\0',BUFFERSIZE);
				
                }
		kill(pid, 9);
		waitpid(pid,NULL,0);
             
        }
        return 1;
}


/*
 *Search all the wifi list that can be used
 *the return value is the num of the wiif can be used
 */

int SearchWiFi(struct WiFi_Info *scan)
{
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
                memset(readbuffer,'\0',BUFFERSIZE);
		while((result = read(fd[0],readbuffer,BUFFERSIZE))>0)
		{
//////////////////////////////////////////////////////////////////////
			//niezhi add in tite
			if(strstr(readbuffer,"Scan completed"))
			{
				ret = GetWiFiStr(readbuffer,scan);
			}
			else
			{
				ret = 0;
			}
////////////////////////////////////////////////////////////////
			kill(pid,9);
			waitpid(pid,NULL,0);
			return ret;
                }
        }
	
}

int GetLinkSSID(N_Info_t *pNetInfo)
{
	struct stat Wireless_Stat;
	int iSize;
	int fd;
	char *pSSID;
	int i=0;
	system("cat /proc/net/wlan/info >/var/network/wifi_info");	
	fd = open("/var/network/wifi_info",O_RDONLY);
	if(fd == -1)
	{
		gNetInfo.iStatus_Wireless = 0;
		return -1;
	}
	fstat(fd,&Wireless_Stat);
	iSize = Wireless_Stat.st_size;
	szFileBuffer =(char*)malloc(iSize);
	if(szFileBuffer == NULL)
	{
		syslog(LOG_USER|LOG_INFO,"malloc is fail\n");
	}

	read(fd,szFileBuffer,iSize);
	if((pSSID = strstr(szFileBuffer,"ESSID=\"")) != NULL)
	{
		pSSID=pSSID+7;
		while(*pSSID != '\"')
		{
			pNetInfo->szLink_SSID[i]=*pSSID;
			pSSID++;
			i++;
		}

		free(szFileBuffer);
		szFileBuffer = NULL;
		return 0;
	}
	return -1;
	
}


int GetDNSStr(struct NetStatus *Status)
{
	struct stat Wireless_Stat;
	int iSize;
	int fd;
	int i = 0,j = 0;
	char match[20] = {'\0'};
	char *p = NULL;
	system("cat /etc/resolv.conf >/var/network/dns_info");
	fd = open("/var/network/dns_info",O_RDONLY);
	if(fd == -1)
	{
		gNetInfo.iStatus_Wireless = 0;
		return -1;
	}
	fstat(fd,&Wireless_Stat);
	iSize = Wireless_Stat.st_size;
	szFileBuffer =(char*)malloc(iSize);
	if(szFileBuffer == NULL)
	{
		syslog(LOG_USER|LOG_INFO,"malloc is fail\n");
	}

	read(fd,szFileBuffer,iSize);
	
	p = strtok(szFileBuffer,"\n"); //·Ö½â×Ö·û´®
	while((p = strtok(NULL,"\n")) != NULL)
	{
		if(strlen(p) > 0)
		{
			if(strstr(p,"nameserver "))
			{
				GetModeStr(p,"nameserver ",'\0',Status->DNSServer[i]);
				i++;
			}
			if(i == 2)
				break;
		}
	}


}

int GetGateWay(const char *Name,struct NetStatus *Status)
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
        char match[20];
	char readbuffer[BUFFERSIZE];
        int result;
	int i;
	char *p;
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
                strcpy(command,"route |grep default|grep ");
		strcat(command,Name);
		strcat(command,"|awk \'{print $2}\'");
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
                memset(readbuffer,'\0',BUFFERSIZE);
                if((result = read(fd[0],readbuffer,BUFFERSIZE))>0)
		{
			strcpy(Status->GateWay,readbuffer);		
			
		}
		kill(pid,9);
		waitpid(pid,NULL,0);
	}	
}


void CloseInternet(void)
{
	system("killall udhcpc");
	system("ifconfig eth0 down");

}


int ConnectInternet(void)
{
	int ret;
	CloseInternet();
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
	

}



void CloseWifi(void)
{
	int fd;
	system("killall udhcpc");
	system("iwpriv eth1 deauth");
}

int ConnectWifiAuto(void)
{
	int ret;
	CloseWifi();
	ret = system("iwconfig eth1 key off essid any");
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



int ConnectWifi(char *pSSID, char *pPassword)
{

	int ret;
	int i;
	int keynum;
	char cmd_password[60] = "iwconfig eth1 key ";
	char cmd_every_password[60];
	char cmd_id[60] = "iwconfig eth1 essid ";
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
}

void init_daemon(void)
{
	int pid;
	int i;
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	if(pid = fork())
		exit(0);
	else if(pid < 0)
		exit(1);
	setsid();
	signal(SIGHUP,SIG_IGN);
	if(pid=fork())
		exit(0);
	else if(pid < 0)
		exit(1);
	for(i = 0; i<NOFILE; ++i)
		close(1);
	chdir("/tmp");
	umask(0);
	return;
}	

int Get_Net_Status(int flag)
{
	int ret;
	N_Info_t NetInfo_Tmp;
	ReadNetInfo(&NetInfo_Tmp);

	if(flag == 0)
	{
		if(NetInfo_Tmp.iDhcp_Wire == 1)
		{
			ret = system("ping -I -c 1 eth0 ipradio1.catchmedia.com");
			if(ret == 0)
			{
				gNetInfo.iStatus_Wire = 1;
				return 0;
			}
			else
			{
				gNetInfo.iStatus_Wire = 0;
				memset(&gNetInfo,'\0',sizeof(N_Info_t));
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	else
	{
/*
		ret = system("ping -I -c 1 eth1 ipradio1.catchmedia.com");
		if(ret == 0)
		{
			gNetInfo.iStatus_Wireless = 1;
			return 0;
		}
		else
		{
			gNetInfo.iStatus_Wireless = 0;
			return -1;
		}
*/
		struct stat Wireless_Stat;
		int iSize;
		int fd;
		system("cat /proc/net/wlan/info >/var/network/wifi_info");	
		fd = open("/var/network/wifi_info",O_RDONLY);
		if(fd == -1)
		{
			gNetInfo.iStatus_Wireless = 0;
			return -1;
		}
		fstat(fd,&Wireless_Stat);
		iSize = Wireless_Stat.st_size;
		szFileBuffer =(char*)malloc(iSize);
		if(szFileBuffer == NULL)
		{
			syslog(LOG_USER|LOG_INFO,"malloc is fail\n");
		}

		read(fd,szFileBuffer,iSize);
		if(strstr(szFileBuffer,"Connected"))
		{
			free(szFileBuffer);
			szFileBuffer = NULL;
			if(NetInfo_Tmp.iDhcp_Wireless == 1)
			{
				gNetInfo.iStatus_Wireless = 1;
				return 0;
			}
		}
		else if(strstr(szFileBuffer,"Disconnected"))
		{
			free(szFileBuffer);
			szFileBuffer = NULL;
			gNetInfo.iStatus_Wireless = 0;
			memset(&gNetInfo,'\0',sizeof(N_Info_t));
			return -1;
		}
		close(fd);
	}
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

int main()
{
	struct stat Wireless_Stat;
	int iSize;
	int fd;
	int fifo_fd;
	int ret;
	int iSearchApNum = 0;
	int iConnect_Flag = 255;
	char *pFifo_rBuffer = NULL;
	int len;
	int i;
	char *szSSID = NULL;
	char *szPassword = NULL;
	int iConnect_flag = 0;	
	memset(&gNetInfo,'\0',sizeof(N_Info_t));

	init_daemon();
	SaveNetInfo(&gNetInfo);		
	openlog("daemon_net",LOG_PID,LOG_DAEMON);
	while(1)
	{
		memset(&gNetInfo,'\0',sizeof(N_Info_t));
		ReadNetInfo(&gNetInfo);
#ifdef NET_DEBUG
		/*
		syslog(LOG_USER|LOG_INFO,"szLink_SSID:%s\n",gNetInfo.szLink_SSID);
		for(i = 0; i < iSearchApNum; i++)
		{	
			syslog(LOG_USER|LOG_INFO,"SSID:%s\n",gNetInfo.Scan[i].SSID);
			syslog(LOG_USER|LOG_INFO,"iSignal:%d\n",gNetInfo.Scan[i].iSignal);
		}
		syslog(LOG_USER|LOG_INFO,"ip0:%s\n",gNetInfo.Status[0].IPAddr);
		syslog(LOG_USER|LOG_INFO,"SubNetMask0:%s\n",gNetInfo.Status[0].SubNetMask);
		syslog(LOG_USER|LOG_INFO,"GateWay0:%s\n",gNetInfo.Status[0].GateWay);
		syslog(LOG_USER|LOG_INFO,"Mac0:%s\n",gNetInfo.Status[0].MACAddr);
		syslog(LOG_USER|LOG_INFO,"0dns0:%s\n",gNetInfo.Status[0].DNSServer[0]);
		syslog(LOG_USER|LOG_INFO,"0dns1:%s\n",gNetInfo.Status[0].DNSServer[1]);
		syslog(LOG_USER|LOG_INFO,"ip1:%s\n",gNetInfo.Status[1].IPAddr);
		syslog(LOG_USER|LOG_INFO,"SubNetMask1:%s\n",gNetInfo.Status[1].SubNetMask);
		syslog(LOG_USER|LOG_INFO,"GateWay1:%s\n",gNetInfo.Status[1].GateWay);
		syslog(LOG_USER|LOG_INFO,"Mac1:%s\n",gNetInfo.Status[1].MACAddr);
		syslog(LOG_USER|LOG_INFO,"1dns0:%s\n",gNetInfo.Status[1].DNSServer[0]);
		syslog(LOG_USER|LOG_INFO,"1dns1:%s\n",gNetInfo.Status[1].DNSServer[1]);*/
#endif
		Get_Net_Status(0);
		Get_Net_Status(1);
		iSearchApNum = SearchWiFi(gNetInfo.Scan);
		gNetInfo.iSearchApNum = iSearchApNum;

		if(gNetInfo.iStatus_Wire == 0 && gNetInfo.iStatus_Wireless == 0)
		{
			if(iConnect_flag == 1)
			{
				iConnect_flag = 0;
				fd = open("/var/mmc/pid",O_RDONLY);
				if(fd == -1)
				{
					gNetInfo.iStatus_Wireless = 0;
					return -1;
				}
				memset(&Wireless_Stat,'\0',sizeof(struct stat));
				fstat(fd,&Wireless_Stat);
				iSize = Wireless_Stat.st_size;
				szFileBuffer =(char*)malloc(iSize);
				if(szFileBuffer == NULL)
				{
					syslog(LOG_USER|LOG_INFO,"malloc is fail\n");
				}

				read(fd,szFileBuffer,iSize);
				kill(atoi(szFileBuffer),SIG_NET);	
				syslog(LOG_USER|LOG_INFO,"send signal\n");
				free(szFileBuffer);
				szFileBuffer = NULL;
			}
			pFifo_rBuffer =(char*)malloc(1);
			if(pFifo_rBuffer == NULL)
			{
				exit(-1);
			}
			fifo_fd = open(NET_FIFO,O_RDONLY|O_NONBLOCK);
			if(fifo_fd == -1)
			{
				continue;
			}	
			syslog(LOG_USER|LOG_INFO,"read is ok!\n");
			if(read(fifo_fd,pFifo_rBuffer,1))
			{
				iConnect_Flag = *pFifo_rBuffer;
				if(iConnect_Flag == 0)
				{
					free(pFifo_rBuffer);
					pFifo_rBuffer =NULL;
					ret = ConnectInternet();
					if(ret == 0)
					{
						gNetInfo.iDhcp_Wire = 1;
					}
					else
					{
						gNetInfo.iDhcp_Wire = 0;
					}
						
				}
				else if(iConnect_Flag == 1)
				{
					read(fifo_fd,pFifo_rBuffer,1);
					len = *pFifo_rBuffer;
					szSSID = (char*)malloc(len+1);
					if(szSSID == NULL)
					{
						exit(-1);
					}
					if(read(fifo_fd,szSSID,len))
					{
						*(szSSID+len) = '\0';
						if(read(fifo_fd,pFifo_rBuffer,1))
						{
							len = *pFifo_rBuffer;
							if(len == 0)
							{
								szPassword = NULL;
								ret = ConnectWifi(szSSID,szPassword);		
								if(ret == 0)
								{
									gNetInfo.iDhcp_Wireless = 1;
								}
								else
								{
									gNetInfo.iDhcp_Wireless = 0;
								}
							}
							else
							{
								szPassword=(char*)malloc(len+1);
								if(szPassword == NULL)
								{
									exit(-1);
								}
								if(read(fifo_fd,szPassword,len))
								{
									*(szPassword+len) = '\0';
									ret = ConnectWifi(szSSID,szPassword);		
									free(szSSID);
									free(szPassword);
									szSSID = NULL;
									szPassword = NULL;
									if(ret == 0)
									{
										gNetInfo.iDhcp_Wireless = 1;
									}
									else
									{
										gNetInfo.iDhcp_Wireless = 0;
									}
										
								}
							}
							free(pFifo_rBuffer);
							pFifo_rBuffer = NULL;
						}
						
					
					}
				}
				else if(iConnect_Flag == 2)
				{
					free(pFifo_rBuffer);
					pFifo_rBuffer =NULL;
					ret =ConnectWifiAuto();
					if(ret == 0)
					{
						gNetInfo.iDhcp_Wireless = 1;
					}
					else
					{
						gNetInfo.iDhcp_Wireless = 0;
					}
				}
			}
			sleep(1);
				
		}
		if(gNetInfo.iStatus_Wire == 1)
		{
			if(iConnect_flag == 0)
			{
				iConnect_flag = 1;
				fd = open("/var/mmc/pid",O_RDONLY);
				if(fd == -1)
				{
					gNetInfo.iStatus_Wireless = 0;
					return -1;
				}
				memset(&Wireless_Stat,'\0',sizeof(struct stat));
				fstat(fd,&Wireless_Stat);
				iSize = Wireless_Stat.st_size;
				szFileBuffer =(char*)malloc(iSize);
				if(szFileBuffer == NULL)
				{
					syslog(LOG_USER|LOG_INFO,"malloc is fail\n");
				}

				read(fd,szFileBuffer,iSize);
				kill(atoi(szFileBuffer),SIG_NET);	
				syslog(LOG_USER|LOG_INFO,"send signal\n");
				free(szFileBuffer);
				szFileBuffer = NULL;
			}

			GetNetStatus("eth0",&gNetInfo.Status[0]);
			GetGateWay("eth0",&gNetInfo.Status[0]);
		}
		if(gNetInfo.iStatus_Wireless == 1)
		{
			int ret;
			GetLinkSSID(&gNetInfo);
			if(iConnect_flag == 0)
			{
				iConnect_flag = 1;
				fd = open("/var/mmc/pid",O_RDONLY);
				if(fd == -1)
				{
					gNetInfo.iStatus_Wireless = 0;
					return -1;
				}
				memset(&Wireless_Stat,'\0',sizeof(struct stat));
				fstat(fd,&Wireless_Stat);
				iSize = Wireless_Stat.st_size;
				szFileBuffer =(char*)malloc(iSize);
				if(szFileBuffer == NULL)
				{
					syslog(LOG_USER|LOG_INFO,"malloc is fail\n");
				}

				read(fd,szFileBuffer,iSize);
				kill(atoi(szFileBuffer),SIG_NET);	
				syslog(LOG_USER|LOG_INFO,"send signal\n");
				free(szFileBuffer);
				szFileBuffer = NULL;
			}
			
			for(i = 0; i<iSearchApNum; i++)
			{
				if((ret = strcmp(gNetInfo.Scan[i].SSID,gNetInfo.szLink_SSID)) == 0)
				{
					gNetInfo.iSignal = gNetInfo.Scan[i].iSignal;
					break;
				}
			}
			GetNetStatus("eth1",&gNetInfo.Status[1]);
			GetGateWay("eth1",&gNetInfo.Status[1]);
		}

		SaveNetInfo(&gNetInfo);		
		sleep(1);
	}
	closelog();
	return;
}

