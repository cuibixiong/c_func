#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>

#include <linux/rtc.h>
#include "time_interface.h"

#define RTC_NODE  "/dev/rtc"
#define DEBUG

#ifdef DEBUG
#define DPRINTF  printf
#else
#define DPRINTF 
#endif

#define LINE_BUF_LEN 1024
#define TARGET_URL_LEN 512

int rtc_set_cur_time(int fd, struct rtc_time *p_rtc_time)
{
  int ret;
  ret = ioctl(fd, RTC_SET_TIME, p_rtc_time);
  return ret;
}


int rtc_get_cur_time(int fd, struct rtc_time *p_rtc_time)
{
  int ret;
  ret = ioctl(fd, RTC_RD_TIME, p_rtc_time);
  return ret;
}

int rtc_set_alarm_time_and_suspend(int fd, struct rtc_time *p_rtc_time)
{
  int ret;
  ret = ioctl(fd, RTC_ALM_SET, p_rtc_time);
  if(ret == 0)
    system("echo 0 > /proc/sys/pm/suspend");

  return ret;
}


/* return value : 0 success ; not 0 error */
int GetRTCTime(Time_Inter_t* pstTime)
{
  int ret;
  int rtcfd;
  struct rtc_time my_rtc_time;

  rtcfd = open(RTC_NODE,O_RDONLY);
  if(rtcfd < 0) {
    DPRINTF("open rtc error!\n");
    return -1;
  }

  ret = rtc_get_cur_time(rtcfd, &my_rtc_time);
  if(ret < 0) {
    DPRINTF("ioctl RTC_GET_TIME rtc error!\n");
    close(rtcfd);
    return -1;
  }
  //  system("hwclock -s");
  //  DPRINTF("year = %02d\n",my_rtc_time.tm_year);
  DPRINTF("\n\nCurrent RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",my_rtc_time.tm_mday, my_rtc_time.tm_mon + 1, my_rtc_time.tm_year + 1900,my_rtc_time.tm_hour, my_rtc_time.tm_min, my_rtc_time.tm_sec);
  
  pstTime->tm_sec = my_rtc_time.tm_sec;
  pstTime->tm_min = my_rtc_time.tm_min;
  pstTime->tm_hour = my_rtc_time.tm_hour;
  pstTime->tm_day = my_rtc_time.tm_mday;
  pstTime->tm_month = my_rtc_time.tm_mon+1;
  pstTime->tm_year = my_rtc_time.tm_year+1900;

  close(rtcfd);

  return 0;

}

/* return value : 0 success ; not 0 error */
int GetSysTime(Time_Inter_t* pstTime)
{
  int ret;
  time_t mytime;
  struct tm my_rtc_time, * p_my_time;

  mytime = time(&mytime);
  if(mytime == (time_t)-1)
    return -1;

  p_my_time = localtime(&mytime);
  if(p_my_time == NULL)
    return -1;

  my_rtc_time = *p_my_time;

  DPRINTF("\n\nCurrent System date/time is %d-%d-%d, %02d:%02d:%02d.\n",my_rtc_time.tm_mday, my_rtc_time.tm_mon + 1, my_rtc_time.tm_year + 1900,my_rtc_time.tm_hour, my_rtc_time.tm_min, my_rtc_time.tm_sec);
  
  pstTime->tm_sec = my_rtc_time.tm_sec;
  pstTime->tm_min = my_rtc_time.tm_min;
  pstTime->tm_hour = my_rtc_time.tm_hour;
  pstTime->tm_day = my_rtc_time.tm_mday;
  pstTime->tm_month = my_rtc_time.tm_mon+1;
  pstTime->tm_year = my_rtc_time.tm_year+1900;


  return 0;

}

int SetSysTime(Time_Inter_t* pstTime)
{
  int ret;
  int rtcfd;
  struct rtc_time my_rtc_time;

  rtcfd = open(RTC_NODE,O_RDONLY);
  if(rtcfd < 0) {
    DPRINTF("open rtc error!\n");
    return -1;
  }

  my_rtc_time.tm_sec = pstTime->tm_sec; // = my_rtc_time.tm_sec;
  my_rtc_time.tm_min = pstTime->tm_min; // = my_rtc_time.tm_min;
  my_rtc_time.tm_hour = pstTime->tm_hour; // = my_rtc_time.tm_hour;
  my_rtc_time.tm_mday = pstTime->tm_day; // = my_rtc_time.tm_mday;
  my_rtc_time.tm_mon = pstTime->tm_month-1; // = my_rtc_time.tm_mon+1;
  my_rtc_time.tm_year = pstTime->tm_year - 1900; //= my_rtc_time.tm_year+1900;
  my_rtc_time.tm_wday = -1;
  my_rtc_time.tm_yday = -1;
  my_rtc_time.tm_isdst = -1;

  ret = rtc_set_cur_time(rtcfd, &my_rtc_time);
  if(ret < 0) {
    DPRINTF("ioctl RTC_GET_TIME rtc error!\n");
    close(rtcfd);
    return -1;
  }

  //  DPRINTF("year = %02d\n",my_rtc_time.tm_year);
  DPRINTF("\n\nCurrent RTC & System date/time is %d-%d-%d, %02d:%02d:%02d.\n",my_rtc_time.tm_mday, my_rtc_time.tm_mon + 1, my_rtc_time.tm_year + 1900,my_rtc_time.tm_hour, my_rtc_time.tm_min, my_rtc_time.tm_sec);
  
  close(rtcfd);

  return 0;
  
}

int SetSleepTime(Time_Inter_t* pstTime)
{
  int ret;
  int rtcfd;
  struct rtc_time my_rtc_time;

  rtcfd = open(RTC_NODE,O_RDONLY);
  if(rtcfd < 0) {
    DPRINTF("open rtc error!\n");
    return -1;
  }

  my_rtc_time.tm_sec = pstTime->tm_sec; // = my_rtc_time.tm_sec;
  my_rtc_time.tm_min = pstTime->tm_min; // = my_rtc_time.tm_min;
  my_rtc_time.tm_hour = pstTime->tm_hour; // = my_rtc_time.tm_hour;
  my_rtc_time.tm_mday = pstTime->tm_day; // = my_rtc_time.tm_mday;
  my_rtc_time.tm_mon = pstTime->tm_month-1; // = my_rtc_time.tm_mon+1;
  my_rtc_time.tm_year = pstTime->tm_year - 1900; //= my_rtc_time.tm_year+1900;
  my_rtc_time.tm_wday = -1;
  my_rtc_time.tm_yday = -1;
  my_rtc_time.tm_isdst = -1;


  ret = rtc_set_alarm_time_and_suspend(rtcfd, &my_rtc_time);
  if(ret < 0) {
    DPRINTF("ioctl RTC_GET_TIME rtc error!\n");
    close(rtcfd);
    return -1;
  }

  //  DPRINTF("year = %02d\n",my_rtc_time.tm_year);
  DPRINTF("\n\nCurrent RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",my_rtc_time.tm_mday, my_rtc_time.tm_mon + 1, my_rtc_time.tm_year + 1900,my_rtc_time.tm_hour, my_rtc_time.tm_min, my_rtc_time.tm_sec);
  
  close(rtcfd);

  return 0;

}

/*success : return 0 ; failed : return not0 */
int NetTimerSync(int iCityNum)
{
  int ret = -1,retcode;
  FILE* streamfd;
  unsigned char *sbuf, *tbuf;
  unsigned char *urlbuf, *dbuf;
  struct rtc_time my_rtc_time;

  streamfd = fopen("/etc/ntp.conf","r");
  if(streamfd == NULL) {
    DPRINTF("open ntp.conf error!\n");
    return -1;
  }
  
  sbuf = malloc(LINE_BUF_LEN);
  if(sbuf == NULL) {
    DPRINTF("malloc line buffer error!\n");
    fclose(streamfd);
    return ret;
  }

  urlbuf = malloc(TARGET_URL_LEN);
  if(urlbuf == NULL) {
    DPRINTF("malloc target buffer error!\n");
    free(sbuf);
    fclose(streamfd);
    return ret;
  }

  while((sbuf = fgets(sbuf, LINE_BUF_LEN, streamfd)) != NULL) {

    if(strncmp(sbuf, "server", 6) != 0)
      continue;
    
    /* get a valid line to contained server URL */
    if(*(sbuf+6) == 0x9 || *(sbuf+6) == ' ') {
      tbuf = sbuf+6;
    } else {
      continue;
    }

    while(*tbuf == ' ' || *tbuf == 0x9) {
      tbuf++;
    }
    dbuf = urlbuf+8;
    strcpy(urlbuf,"ntpdate ");

    while((char)*tbuf != ' ' && (char)*tbuf != EOF && (char)*tbuf != 0x9 && (char)*tbuf != 0xa) {
      *dbuf++ = *tbuf++;
    }
    *dbuf = 0;
    
    retcode = system(urlbuf);
    if(retcode == 0) {
      ret = 0;
      break;
    }
    DPRINTF("%s = %x\n",urlbuf,ret);
    
  }

  fclose(streamfd);
  free(sbuf);
  free(urlbuf);
  return ret;
}

int main()
{
  int ret;
  Time_Inter_t my_test_time;
  Time_Inter_t my_rtc_time;
  /* when sys startup, we should read time from hardware rtc */
  ret = GetRTCTime(&my_rtc_time);
  if(ret != 0) {
    DPRINTF("get rtc time error\n");
    return -1;
  }

  /* get and display system time */
  ret = GetSysTime(&my_test_time);
  if(ret != 0) {
    DPRINTF("get sys time error\n");
    return -1;
  }

  /* according from rtc time, we set system time and rtc time */
  ret = SetSysTime(&my_rtc_time);
  if(ret != 0) {
    DPRINTF("set sys & rtc time error\n");
    return -1;
  }

  /* after set sys & rtc time, we get the sys time and display It */
  ret = GetSysTime(&my_test_time);
  if(ret != 0) {
    DPRINTF("get sys time error\n");
    return -1;
  }
  
  /* sync system time from network, we get greenwich time as system time */
  ret = NetTimerSync(0);

  /* get & display modified system time */
  ret = GetSysTime(&my_test_time);
  if(ret != 0) {
    DPRINTF("get sys time error\n");
    return -1;
  }

  /* should modify the local system time according UTC & timezone info */
  //......

  /* after change the system time , we should call SetSysTime() */


  /* test sleep function */

  ret = GetRTCTime(&my_rtc_time);
  if(ret != 0) {
    DPRINTF("get rtc time error\n");
    return -1;
  }
  if(my_rtc_time.tm_sec < 40)
    my_rtc_time.tm_sec += 10;

  DPRINTF("sleep 10 seconds...\n");

  SetSleepTime(&my_rtc_time);
  
  return 0;
}
#if 0
int main()
{
  struct timeval mytime;
  struct rtc_time my_rtc_time;
  int ret;
  int rtcfd;
  time_t atime;

  rtcfd = open(RTC_NODE,O_RDWR);
  if(rtcfd < 0) {
    DPRINTF("open rtc error!\n");
    return -1;
  }

  ret = rtc_get_cur_time(rtcfd, &my_rtc_time);
  if(ret < 0) {
    DPRINTF("ioctl RTC_GET_TIME rtc error!\n");
    close(rtcfd);
    return -1;
  }
  
  //  DPRINTF("year = %02d\n",my_rtc_time.tm_year);
  DPRINTF("\n\nCurrent RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",my_rtc_time.tm_mday, my_rtc_time.tm_mon + 1, my_rtc_time.tm_year + 1900,my_rtc_time.tm_hour, my_rtc_time.tm_min, my_rtc_time.tm_sec);
  my_rtc_time.tm_sec = 20;
  my_rtc_time.tm_min = 20;
  my_rtc_time.tm_hour = 20;
  my_rtc_time.tm_mday = 20;
  my_rtc_time.tm_mon = 4;
  my_rtc_time.tm_year = 2008-1900;
  my_rtc_time.tm_wday = -1;
  my_rtc_time.tm_yday = -1;
  my_rtc_time.tm_isdst = -1;

#if 0
  ret = rtc_set_cur_time(rtcfd, &my_rtc_time);
  if(ret < 0) {
    DPRINTF("ioctl RTC_SET_TIME rtc error!\n");
    close(rtcfd);
    return -1;
  }
  

  ret = rtc_get_cur_time(rtcfd, &my_rtc_time);
  if(ret < 0) {
    DPRINTF("ioctl RTC_GET_TIME rtc error!\n");
    close(rtcfd);
    return -1;
  }

  DPRINTF("\n\nCurrent RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",my_rtc_time.tm_mday, my_rtc_time.tm_mon + 1, my_rtc_time.tm_year + 1900,my_rtc_time.tm_hour, my_rtc_time.tm_min, my_rtc_time.tm_sec);
  
  //  DPRINTF("year = %d\n",my_rtc_time.tm_year);

  my_rtc_time.tm_min += 1;
  ret = rtc_set_alarm_time_and_suspend(rtcfd, &my_rtc_time);
  if(ret < 0) {
    DPRINTF("ioctl RTC_GET_TIME rtc error!\n");
    close(rtcfd);
    return -1;
  }
#endif
  //  system("echo 0 > /proc/sys/pm/suspend");

  //  mytime.tv_sec = 0x60000000;
  //  mytime.tv_usec = 000;

  //  ret = settimeofday(&mytime,NULL);

  ret = gettimeofday(&mytime,NULL);
  printf("ret = %d\n",ret);
  
  atime = mytime.tv_sec;
  printf("%s\n",ctime(&atime));
  return 0;
}
#endif
