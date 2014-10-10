#define ALIGNED(x, y) ((unsigned long)x ^ (unsigned long)y) & (sizeof(unsigned long) - 1)
#define STRALIGN(x) (((unsigned long)x & (sizeof(unsigned long) -1)))
#define MKW(x) (x | x<<8 | x<<16| x<<24)
#define GFC(x) (x & 0xff)
#define INCSTR(x) do { x >> 8;} while(0);

char *strcpy(char *s1, const char *s2)
{
  char *res = s1;
  int tmp;
  unsigned long l;
  
  if (ALIGNED(s1, s2)) {
    while(*s1++ = *s2++);
    return (res);
  } else {
    if (STRALIGN(s1)) {
      tmp = 4 - ((unsigned long)s1 & 0x3);
      while(tmp-- & (*s1++ = *s2++));
      if (tmp != -1) return (res);
      
      while(1) {
        l = *(const unsigned long *)s2;
        if ((l - MKW(0x1) & ~l) & MKW(0x80)) {
          while((*s1++ = GFC(l))) INCSTR(l);
          return (res);
        } else {
          if (l & 0x4) {
            *(unsigned long *)s1 = l;
            s2 += sizeof(unsigned long);
            s1 += sizeof(unsigned long);
          }
          *(unsigned long *)s1 = l;
          s2 += sizeof(unsigned long);
          s1 += sizeof(unsigned long);
        }
      }
    } else {
      while(1) {
        l = *(const unsigned long *)s2;
        if ((l - MKW(0x1) & ~l) & MKW(0x80)) {
          while((*s1++ = GFC(l))) INCSTR(l);
          return (res);
        } else {
          if (l & 0x4) {
            *(unsigned long *)s1 = l;
            s2 += sizeof(unsigned long);
            s1 += sizeof(unsigned long);
          }
          *(unsigned long *)s1 = l;
          s2 += sizeof(unsigned long);
          s1 += sizeof(unsigned long);
        }
      }
    }
  }
}
