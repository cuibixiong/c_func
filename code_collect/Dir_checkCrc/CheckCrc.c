#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "CheckCrc.h"

unsigned char FileList[MAXFILE][LINEMAX];

const uLongf crc_table[256] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL};

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf) \
  DO1(buf);      \
  DO1(buf);
#define DO4(buf) \
  DO2(buf);      \
  DO2(buf);
#define DO8(buf) \
  DO4(buf);      \
  DO4(buf);

/* ========================================================================= */
uLong crc32(uLong crc, const Byte *buf, uInt len) {
#ifdef DYNAMIC_CRC_TABLE
  if (crc_table_empty) make_crc_table();
#endif
  crc = crc ^ 0xffffffffL;
  while (len >= 8) {
    DO8(buf);
    len -= 8;
  }
  if (len) do {
      DO1(buf);
    } while (--len);
  return crc ^ 0xffffffffL;
}

static int isdir(const char *dir) {
  DIR *pdir;
  pdir = opendir(dir);
  if (!pdir) return 0;  // not dir;
  closedir(pdir);
  return 1;  // is dir
}

int GetFileCRC(const char *filename, uLong *crc) {
  FILE *fp;
  unsigned int size;
  unsigned char *buffer;

  if (!filename || !crc) return -1;

  if (isdir(filename)) {
    printf("%s filename is dir\n", __func__);
    return -1;
  }
  fp = fopen(filename, "rb");
  if (!fp) {
    printf("open [%s] fail\n", filename);
    return -1;
  }
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (size > 4 * 1024 * 1024)  // if size > 2M , then return fail.
  {
    printf("File size > 4M, too big.\n");
    fclose(fp);
    return -2;
  }
  buffer = (unsigned char *)malloc(size + 1);
  if (!buffer) {
    fclose(fp);
    printf("malloc fail\n");
    return -1;
  }

  memset(buffer, '\0', size);
  fread(buffer, size, 1, fp);
  fclose(fp);

  *crc = crc32(0, buffer, size);
  free(buffer);
  buffer = NULL;
  return 0;
}

/***********************************
 *
 * int GetFileList(const char *dir)
 * Description:
 *            Get file name on dir and save to FileList[]
 *
 * return value:
 *           return number of file.  -1 is fail.
 *
 ***********************************/
static int GetFileList(const char *dir) {
  FILE *fp;
  char cmd[512];
  int n = 0, i = 0;
  if (!dir) return -1;
  if (!(isdir(dir)))  // not dir
    return -1;
  for (i = 0; i < MAXFILE; i++) {
    memset(FileList[i], 0, LINEMAX);
  }

  strcpy(cmd, "find ");
  strcat(cmd, dir);
  strcat(cmd, " -type f >");
  strcat(cmd, TEMP_FILE);
  printf("CMD = [%s]\n", cmd);
  system(cmd);

  fp = fopen(TEMP_FILE, "r");
  if (!fp) {
    printf("open tmp~ file fail\n");
    return -1;
  }
  while (fgets(FileList[n], LINEMAX, fp) != NULL) {
    int k;
    for (k = 0; FileList[n][k]; k++) {
      if (FileList[n][k] == '\n') FileList[n][k] = '\0';
    }
    n++;
    if (n == MAXFILE) break;
  }
  fclose(fp);
  unlink(TEMP_FILE);

  return n;
}

struct crc_info_t crc_info = {
    "CRC0", 0x12345678, 0, 0,
};

int compare_record(const void *a, const void *b) {
  struct crc_record_t *aa = (struct crc_reocrd_t *)a;
  struct crc_record_t *bb = (struct crc_reocrd_t *)b;
  if (aa->crc > bb->crc) return 1;
  if (aa->crc == bb->crc) return 0;
  if (aa->crc < bb->crc) return -1;
}

int GenCrcFile(const char *dir, char *crcfile) {
  int i, cnt, ret;
  int fd;
  uLong crc;
  struct crc_record_t *crc_record = NULL;

  if (!dir || !crcfile) return -1;

  unlink(crcfile);
  cnt = GetFileList(dir);
  if (cnt == -1) {
    return -1;
  }
  crc_record = (struct crc_record_t *)malloc(sizeof(struct crc_record_t) * cnt);
  if (!crc_record) {
    return -1;
  }
  memset(crc_record, 0, sizeof(struct crc_record_t) * cnt);

  for (i = 0; i < cnt; i++) {
    dprintf("FileList[%d] = %s\n", i, FileList[i]);
    crc = 0;
    ret = GetFileCRC(FileList[i], &crc);

    /* save filename and crc , then write to file. */
    strcpy(crc_record[i].FileName, FileList[i]);
    crc_record[i].crc = crc;

    if (ret == 0) {
      dprintf("CRC=[ 0x%x ], Filename= [ %s ]\n", crc, FileList[i]);
    } else {
      printf("Filename = [%s] GetFileCrc fail\n", FileList[i]);
      continue;
    }
  }
  crc = crc32(0, (unsigned char *)crc_record,
              (unsigned int)(cnt * sizeof(struct crc_record_t)));
  dprintf("record crc = 0x%x\n", crc);

  /* save crc_info number record and a file crc */
  crc_info.nrecord = cnt;
  crc_info.crc = crc;

  fd = open(crcfile, O_RDWR | O_CREAT, 0777);
  if (fd < 0) {
    printf("create file %s fail\n", crcfile);
    return -1;
  }
  write(fd, &crc_info, sizeof(crc_info));  // write crc info header

#ifdef CRC_DEBUG
  for (i = 0; i < cnt; i++) {
    printf("i=%d, name=%s, crc=0x%x\n", i,
           (struct crc_record_t *)(crc_record + i)->FileName,
           (struct crc_record_t *)(crc_record + i)->crc);
  }
  printf("Sort end\n");
  qsort(crc_record, cnt, sizeof(struct crc_record_t), compare_record);
  for (i = 0; i < cnt; i++) {
    printf("i=%d, name=%s, crc=0x%x\n", i,
           (struct crc_record_t *)(crc_record + i)->FileName,
           (struct crc_record_t *)(crc_record + i)->crc);
  }
#endif

  write(fd, crc_record, (cnt * sizeof(struct crc_record_t)));
  close(fd);
  free((void *)crc_record);
  crc_record = NULL;
  return 0;
}

void DumpCrcInfo(struct crc_info_t *info) {
  if (!info) return;

#ifdef CRC_DEBUG
  printf("Dump Info:\n");
  printf("Info.file = %c%c%c%c\n", info->file[0], info->file[1], info->file[2],
         info->file[3]);
  printf("Info.magic = 0x%x\n", info->magic);
  printf("Info.nrecord = %d\n", info->nrecord);
  printf("Info.crc = 0x%x\n", info->crc);
#endif
}

void DumpCrcRecord(struct crc_record_t *p, int num) {
  int i;
  if (!p) return;

#ifdef CRC_DEBUG
  for (i = 0; i < num; i++) {
    printf("Dump CRC Record: \n");
    printf("Record.FileName = %s\n", p[i].FileName);
    printf("Record.crc = 0x%x\n", p[i].crc);
  }
#endif
}

int DumpCrcFile(const char *filename) {
  FILE *fp;
  int cnt, ret;
  struct crc_record_t *crc_record = NULL;
  struct crc_info_t crc_header;

  if (!filename) return -1;

  fp = fopen(filename, "rb");
  if (!fp) {
    return -1;
  }
  if (fread(&crc_header, sizeof(struct crc_info_t), 1, fp) != 1) {
    fclose(fp);
    return -1;
  }
  if (crc_header.magic != 0x12345678) {
    fclose(fp);
    printf("Not crc file format\n");
    return -1;
  }

  DumpCrcInfo(&crc_header);

  cnt = crc_header.nrecord;
  crc_record = (struct crc_record_t *)malloc(sizeof(struct crc_record_t) * cnt);
  if (!crc_record) {
    printf("malloc fail\n");
    fclose(fp);
    return -1;
  }
  memset(crc_record, 0, sizeof(struct crc_record_t) * cnt);
  ret = fread(crc_record, sizeof(struct crc_record_t), cnt, fp);
  if (ret != cnt) {
    printf("read fail, ret=%d\n", ret);
    fclose(fp);
    free((void *)crc_record);
    return -1;
  }
  fclose(fp);

  DumpCrcRecord(crc_record, cnt);

  free((void *)crc_record);
  crc_record = NULL;
  return 0;
}

int CheckDirCrc(const char *SrcDir, const char *CmpCrcFile) {
  FILE *fp;
  FILE *dfp;
  int i, cnt, ret;
  unsigned char *pname, *crcname;
  int dcnt;
  struct crc_record_t *scrc_record = NULL;
  struct crc_info_t scrc_header;

  struct crc_record_t *dcrc_record = NULL;
  struct crc_info_t dcrc_header;

  if (!SrcDir || !CmpCrcFile) {
    printf("dir or file is null \n");
    return 0;
  }

  if (GenCrcFile(SrcDir, TEMP_FILE) == -1) {
    printf("generate temp file fail \n");
    return 0;
  }

  fp = fopen(CmpCrcFile, "rb");
  if (!fp) {
    printf("open %s file fail \n", CmpCrcFile);
    return 0;
  }
  if (fread(&scrc_header, sizeof(struct crc_info_t), 1, fp) != 1) {
    fclose(fp);
    printf("fread file fail \n");
    return 0;
  }
  if (scrc_header.magic != 0x12345678) {
    fclose(fp);
    printf("crc file not crc format\n");
    return 0;
  }
  dfp = fopen(TEMP_FILE, "rb");
  if (!dfp) {
    ret = 0;
    printf("open temp file fail \n");
    goto closeall;
  }
  if (fread(&dcrc_header, sizeof(struct crc_info_t), 1, dfp) != 1) {
    ret = 0;
    printf("fread temp file fail \n");
    goto closeall;
  }

  // DumpCrcInfo(&scrc_header);
  if ((scrc_header.magic != 0x12345678) || (dcrc_header.magic != 0x12345678)) {
    ret = 0;
    printf("src magic 0x%x != dsrc magic 0x%x \n", scrc_header.magic,
           dcrc_header.magic);
    goto closeall;
  }
  cnt = scrc_header.nrecord;
  dcnt = dcrc_header.nrecord;
  if (cnt != dcnt) {
    ret = 0;
    printf("src record %d != dsrc record %d\n", scrc_header.nrecord,
           dcrc_header.nrecord);
    goto closeall;
  }
  scrc_record =
      (struct crc_record_t *)malloc(sizeof(struct crc_record_t) * cnt);
  if (!scrc_record) {
    printf("malloc  srecord fail\n");
    ret = 0;
    goto closeall;
  }
  dcrc_record =
      (struct crc_record_t *)malloc(sizeof(struct crc_record_t) * cnt);
  if (!dcrc_record) {
    printf("malloc drecord fail\n");
    ret = 0;
    goto closeall;
  }

  memset(scrc_record, 0, sizeof(struct crc_record_t) * cnt);
  memset(dcrc_record, 0, sizeof(struct crc_record_t) * cnt);

  ret = fread(scrc_record, sizeof(struct crc_record_t), cnt, fp);
  if (ret != cnt) {
    printf("read fail, ret=%d\n", ret);
    ret = 0;
    goto closeall;
  }
  ret = fread(dcrc_record, sizeof(struct crc_record_t), cnt, dfp);
  if (ret != cnt) {
    printf("read fail, ret=%d\n", ret);
    ret = 0;
    goto closeall;
  }
  // DumpCrcRecord(scrc_record, cnt);
  // DumpCrcRecord(dcrc_record, cnt);
  for (i = 0; i < cnt; i++) {
    dprintf("src.name [%s] <-> dsrc.name [%s]\n",
            strrchr(scrc_record[i].FileName, '/'),
            strrchr(dcrc_record[i].FileName, '/'));
    if (strcmp(strrchr(scrc_record[i].FileName, '/'),
               strrchr(dcrc_record[i].FileName, '/'))) {
      ret = 0;
      printf("src.name [%s] != dsrc.name [%s]\n",
             strrchr(scrc_record[i].FileName, '/'),
             strrchr(dcrc_record[i].FileName, '/'));
      break;
    }
    if (crcname = strrchr(scrc_record[i].FileName, '/')) {
      if (strstr(dcrc_record[i].FileName, crcname)) {
        printf("dname = [%s], crcname = [%s]\n", dcrc_record[i].FileName,
               crcname);
        continue;  // skip cmp crcfile
      }
    }

    if (scrc_record[i].crc != dcrc_record[i].crc) {
      ret = 0;
      printf("src.crc 0x%x != dsrc.crc 0x%x\n", scrc_record[i].crc,
             dcrc_record[i].crc);
      break;
    }
  }
  dprintf("i == %d, cnt = %d\n", i, cnt);
  if (i == cnt) ret = 1;

closeall:

  fclose(fp);
  fclose(dfp);
  unlink(TEMP_FILE);
  free(scrc_record);
  free(dcrc_record);

  return ret;  // 0: Dir and Crc not same. 1: Dir and crc is same
}

#ifdef CRC_TEST
int main(int argc, char *argv[]) {
  int ret;
  char outcrc[100];
  if (argc != 2) {
    fprintf(stderr, "\nUsage: %s Directory \n\n", argv[0]);
    return 0;
  }
  memset(outcrc, 0, sizeof(outcrc));
  strcpy(outcrc, argv[1]);
  if (outcrc[strlen(outcrc) - 1] != '/') strcat(outcrc, "/");
  strcat(outcrc, CRC_FILE);
  printf("outcrc = %s\n", outcrc);
  GenCrcFile(argv[1], outcrc);
  DumpCrcFile(argv[2]);

#if 0	
    ret  = CheckDirCrc("ddb", argv[2]);
    //	unlink(argv[2]);
    if (ret)
        printf ("Dir is same\n");
    else 
        printf ("Dir not same\n");
#endif
  return ret;
}

#endif
