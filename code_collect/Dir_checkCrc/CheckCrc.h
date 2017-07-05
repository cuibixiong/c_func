#ifndef _CHECK_CRC_H_
#define _CHECK_CRC_H_

#define CRC_DEBUG
#define CRC_TEST

#ifdef CRC_DEBUG
#define dprintf(x...) printf(x);
#else
#define dprintf(x...)
#endif

#define TEMP_FILE "tmp~1.tmp"
#define CRC_FILE "crc~1.bin"

typedef unsigned char Byte;  /* 8 bits */
typedef unsigned int uInt;   /* 16 bits or more */
typedef unsigned long uLong; /* 32 bits or more */
typedef unsigned long uLongf;

#define MAXFILE 20
#define LINEMAX 80

struct crc_info_t {
  unsigned char file[4];  // always "CRC0"
  unsigned int magic;     // always 0x12345678
  unsigned int nrecord;   // number of struct crc_record_t.
  unsigned int crc;  // a whole number of struct crc_record_t record  crc code.
};

struct crc_record_t {
  unsigned char FileName[LINEMAX];
  uLong crc;
};

/*****************************************
*  CRC File format:
*  ____________________________
*  | struct crc_infot_t        |
*  -----------------------------
*  | 1th struct crc_record_t   |
*  -----------------------------
*  | 2th struct crc_record_t   |
*  -----------------------------
*  | ...th struct crc_record_t |
*  -----------------------------
*
******************************************/

/**********************************
* GetFileCRC():
* description: genera crc code for input filename.
*
* output:  crc code.
* return:  <0 : Fail,   =0: OK
*
**********************************/
extern int GetFileCRC(const char *filename, uLong *crc);

/**********************************
* CheckDirCrc():
* description: genera crc file for input SrcDir. and compare CmpCrcFile .
*
* return:  1 : dir same,   0: not same
*
**********************************/
extern int CheckDirCrc(const char *SrcDir, const char *CmpCrcFile);

/**********************************
* GenCrcFile():
* description: genera crc file for input SrcDir.
*
* return:  <0 : fail,   0: OK
*
**********************************/
int GenCrcFile(const char *dir, char *crcfile);

void DumpCrcRecord(struct crc_record_t *p, int num);
void DumpCrcInfo(struct crc_info_t *info);

/********************************************
* DumpCrcFile();
*
* return -1: fail , 0: OK
**********************************************/
int DumpCrcFile(const char *filename);

#endif  //_CHECK_CRC_H_
