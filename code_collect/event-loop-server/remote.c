#include "server.h"
#include "terminal.h"

#include <stdio.h>
#include <string.h>
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/time.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if USE_WIN32API
#include <winsock2.h>
#endif

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif


#if USE_WIN32API
# define INVALID_DESCRIPTOR INVALID_SOCKET
#else
# define INVALID_DESCRIPTOR -1
#endif

/* Extra value for readchar_callback.  */
enum {
  /* The callback is currently not scheduled.  */
  NOT_SCHEDULED = -1
};

/* Status of the readchar callback.
   Either NOT_SCHEDULED or the callback id.  */
static int readchar_callback = NOT_SCHEDULED;

static int readchar (void);
static void reset_readchar (void);
static void reschedule (void);

static int remote_is_stdio = 0;

gdb_fildes_t remote_desc = INVALID_DESCRIPTOR;
gdb_fildes_t listen_desc = INVALID_DESCRIPTOR;

#ifdef USE_WIN32API
# define read(fd, buf, len) recv (fd, (char *) buf, len, 0)
# define write(fd, buf, len) send (fd, (char *) buf, len, 0)
#endif

int
gdb_connected (void)
{
  return remote_desc != INVALID_DESCRIPTOR;
}

/* Return true if the remote connection is over stdio.  */

int
remote_connection_is_stdio (void)
{
  return remote_is_stdio;
}

static void
enable_async_notification (int fd)
{
#if defined(F_SETFL) && defined (FASYNC)
  int save_fcntl_flags;

  save_fcntl_flags = fcntl (fd, F_GETFL, 0);
  fcntl (fd, F_SETFL, save_fcntl_flags | FASYNC);
#if defined (F_SETOWN)
  fcntl (fd, F_SETOWN, getpid ());
#endif
#endif
}

static int
handle_accept_event (int err, gdb_client_data client_data)
{
  struct sockaddr_in sockaddr;
  socklen_t tmp;

  if (debug_threads)
    fprintf (stderr, "handling possible accept event\n");

  tmp = sizeof (sockaddr);
  remote_desc = accept (listen_desc, (struct sockaddr *) &sockaddr, &tmp);
  if (remote_desc == -1)
    perror_with_name ("Accept failed");

  /* Enable TCP keep alive process. */
  tmp = 1;
  setsockopt (remote_desc, SOL_SOCKET, SO_KEEPALIVE,
	      (char *) &tmp, sizeof (tmp));

  /* Tell TCP not to delay small packets.  This greatly speeds up
     interactive response. */
  tmp = 1;
  setsockopt (remote_desc, IPPROTO_TCP, TCP_NODELAY,
	      (char *) &tmp, sizeof (tmp));

#ifndef USE_WIN32API
  signal (SIGPIPE, SIG_IGN);	/* If we don't do this, then gdbserver simply
				   exits when the remote side dies.  */
#endif

  if (run_once)
    {
#ifndef USE_WIN32API
      close (listen_desc);		/* No longer need this */
#else
      closesocket (listen_desc);	/* No longer need this */
#endif
    }

  /* Even if !RUN_ONCE no longer notice new connections.  Still keep the
     descriptor open for add_file_handler to wait for a new connection.  */
  delete_file_handler (listen_desc);

  /* Convert IP address to string.  */
  fprintf (stderr, "Remote Connect from host %s\n",
	   inet_ntoa (sockaddr.sin_addr));

  enable_async_notification (remote_desc);

  /* Register the event loop handler.  */
  add_file_handler (remote_desc, handle_serial_event, NULL);

  /* We have a new GDB connection now.  If we were disconnected
     tracing, there's a window where the target could report a stop
     event to the event loop, and since we have a connection now, we'd
     try to send vStopped notifications to GDB.  But, don't do that
     until GDB as selected all-stop/non-stop, and has queried the
     threads' status ('?').  */
  target_async (0);

  return 0;
}

/* Prepare for a later connection to a remote debugger.
   NAME is the filename used for communication.  */
void
remote_prepare (char *name)
{
  char *port_str;
#ifdef USE_WIN32API
  static int winsock_initialized;
#endif
  int port;
  struct sockaddr_in sockaddr;
  socklen_t tmp;
  char *port_end;

  remote_is_stdio = 0;
  if (strcmp (name, STDIO_CONNECTION_NAME) == 0)
    {
      /* We need to record fact that we're using stdio sooner than the
	 call to remote_open so start_inferior knows the connection is
	 via stdio.  */
      remote_is_stdio = 1;
      transport_is_reliable = 1;
      return;
    }

  port_str = strchr (name, ':');
  if (port_str == NULL)
    {
      transport_is_reliable = 0;
      return;
    }

  port = strtoul (port_str + 1, &port_end, 10);
  if (port_str[1] == '\0' || *port_end != '\0')
    fatal ("Bad port argument: %s", name);

#ifdef USE_WIN32API
  if (!winsock_initialized)
    {
      WSADATA wsad;

      WSAStartup (MAKEWORD (1, 0), &wsad);
      winsock_initialized = 1;
    }
#endif

  listen_desc = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_desc == -1)
    perror_with_name ("Can't open socket");

  /* Allow rapid reuse of this port. */
  tmp = 1;
  setsockopt (listen_desc, SOL_SOCKET, SO_REUSEADDR, (char *) &tmp,
	      sizeof (tmp));

  sockaddr.sin_family = PF_INET;
  sockaddr.sin_port = htons (port);
  sockaddr.sin_addr.s_addr = INADDR_ANY;

  if (bind (listen_desc, (struct sockaddr *) &sockaddr, sizeof (sockaddr))
      || listen (listen_desc, 1))
    perror_with_name ("Can't bind address");

  transport_is_reliable = 1;
}

/* Open a connection to a remote debugger.
   NAME is the filename used for communication.  */

void
remote_open (char *name)
{
  char *port_str;

  port_str = strchr (name, ':');
#ifdef USE_WIN32API
  if (port_str == NULL)
    error ("Only <host>:<port> is supported on this platform.");
#endif

  if (strcmp (name, STDIO_CONNECTION_NAME) == 0)
    {
      fprintf (stderr, "Remote debugging using stdio\n");

      /* Use stdin as the handle of the connection.
	 We only select on reads, for example.  */
      remote_desc = fileno (stdin);

      enable_async_notification (remote_desc);

      /* Register the event loop handler.  */
      add_file_handler (remote_desc, handle_serial_event, NULL);
    }
#ifndef USE_WIN32API
  else if (port_str == NULL)
    {
      struct stat statbuf;

      if (stat (name, &statbuf) == 0
	  && (S_ISCHR (statbuf.st_mode) || S_ISFIFO (statbuf.st_mode)))
	remote_desc = open (name, O_RDWR);
      else
	{
	  errno = EINVAL;
	  remote_desc = -1;
	}

      if (remote_desc < 0)
	perror_with_name ("Could not open remote device");

#ifdef HAVE_TERMIOS
      {
	struct termios termios;
	tcgetattr (remote_desc, &termios);

	termios.c_iflag = 0;
	termios.c_oflag = 0;
	termios.c_lflag = 0;
	termios.c_cflag &= ~(CSIZE | PARENB);
	termios.c_cflag |= CLOCAL | CS8;
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 0;

	tcsetattr (remote_desc, TCSANOW, &termios);
      }
#endif

#ifdef HAVE_TERMIO
      {
	struct termio termio;
	ioctl (remote_desc, TCGETA, &termio);

	termio.c_iflag = 0;
	termio.c_oflag = 0;
	termio.c_lflag = 0;
	termio.c_cflag &= ~(CSIZE | PARENB);
	termio.c_cflag |= CLOCAL | CS8;
	termio.c_cc[VMIN] = 1;
	termio.c_cc[VTIME] = 0;

	ioctl (remote_desc, TCSETA, &termio);
      }
#endif

#ifdef HAVE_SGTTY
      {
	struct sgttyb sg;

	ioctl (remote_desc, TIOCGETP, &sg);
	sg.sg_flags = RAW;
	ioctl (remote_desc, TIOCSETP, &sg);
      }
#endif

      fprintf (stderr, "Remote debugging using %s\n", name);

      enable_async_notification (remote_desc);

      /* Register the event loop handler.  */
      add_file_handler (remote_desc, handle_serial_event, NULL);
    }
#endif /* USE_WIN32API */
  else
    {
      int port;
      socklen_t len;
      struct sockaddr_in sockaddr;

      len = sizeof (sockaddr);
      if (getsockname (listen_desc,
		       (struct sockaddr *) &sockaddr, &len) < 0
	  || len < sizeof (sockaddr))
	perror_with_name ("Can't determine port");
      port = ntohs (sockaddr.sin_port);

      fprintf (stderr, "Listening on port %d\n", port);
      fflush (stderr);

      /* Register the event loop handler.  */
      add_file_handler (listen_desc, handle_accept_event, NULL);
    }
}

void
remote_close (void)
{
  delete_file_handler (remote_desc);

#ifdef USE_WIN32API
  closesocket (remote_desc);
#else
  if (! remote_connection_is_stdio ())
    close (remote_desc);
#endif
  remote_desc = INVALID_DESCRIPTOR;

  reset_readchar ();
}

/* Convert hex digit A to a number.  */

static int
fromhex (int a)
{
  if (a >= '0' && a <= '9')
    return a - '0';
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else
    error ("Reply contains invalid hex digit");
  return 0;
}

static const char hexchars[] = "0123456789abcdef";

static int
ishex (int ch, int *val)
{
  if ((ch >= 'a') && (ch <= 'f'))
    {
      *val = ch - 'a' + 10;
      return 1;
    }
  if ((ch >= 'A') && (ch <= 'F'))
    {
      *val = ch - 'A' + 10;
      return 1;
    }
  if ((ch >= '0') && (ch <= '9'))
    {
      *val = ch - '0';
      return 1;
    }
  return 0;
}

/* Convert number NIB to a hex digit.  */

static int
tohex (int nib)
{
  if (nib < 10)
    return '0' + nib;
  else
    return 'a' + nib - 10;
}
