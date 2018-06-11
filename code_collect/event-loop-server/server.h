#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <setjmp.h>

#include <string.h>

#include <alloca.h>
/* On some systems such as MinGW, alloca is declared in malloc.h
   (there is no alloca.h).  */
#include <malloc.h>

#if !HAVE_DECL_STRERROR
#ifndef strerror
extern char *strerror (int);	/* X3.159-1989  4.11.6.2 */
#endif
#endif

#if !HAVE_DECL_PERROR
#ifndef perror
extern void perror (const char *);
#endif
#endif

#if !HAVE_DECL_VASPRINTF
extern int vasprintf(char **strp, const char *fmt, va_list ap);
#endif
#if !HAVE_DECL_VSNPRINTF
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

/* Define underscore macro, if not available, to be able to use it inside
   code shared with gdb in common directory.  */
#ifndef _
#define _(String) (String)
#endif

#define PBUFSIZ 16384

#if USE_WIN32API
#include <winsock2.h>
typedef SOCKET remote_fildes_t;
#else
typedef int remote_fildes_t;
#endif

/* Functions from event-loop.c.  */
typedef void *remote_client_data;
typedef int (handler_func) (int, remote_client_data);
typedef int (callback_handler_func) (remote_client_data);

extern void delete_file_handler (remote_fildes_t fd);
extern void add_file_handler (remote_fildes_t fd, handler_func *proc,
			      remote_client_data client_data);
extern int append_callback_event (callback_handler_func *proc,
				   remote_client_data client_data);
extern void delete_callback_event (int id);

extern void start_event_loop (void);
extern void initialize_event_loop (void);

/* Functions from server.c.  */
extern int handle_serial_event (int err, remote_client_data client_data);
extern int handle_target_event (int err, remote_client_data client_data);

#define STDIO_CONNECTION_NAME "stdio"

extern int run_once;
extern int transport_is_reliable;

#endif
