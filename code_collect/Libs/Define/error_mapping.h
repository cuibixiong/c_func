/* Entries may not correspond to actualnumbers. Some entries omitted. */
#define EINVAL 1
#define ENOMEM 2
#define EFAULT 3
/* ... */
#define E2BIG  7
#define EBUSY  8
/* ... */
#define ECHILD 12
/* ... */
 
char *err_strings[] = 
{
  err_strings[0] = "Success",
  err_strings [EINVAL] = "Invalid argument",
  err_strings[ENOMEM] = "Not enough memory",
  err_strings[EFAULT] = "Bad address",
  err_strings[E2BIG ] = "Argument list too long",
  err_strings[EBUSY ] = "Device or resource busy",
  err_strings[ECHILD] = "No child processes"
};
