#include "server.h"
#include "terminal.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define INVALID_DESCRIPTOR -1

/* Extra value for readchar_callback.  */
enum {
  /* The callback is currently not scheduled.  */
  NOT_SCHEDULED = -1
};

/* Status of the readchar callback.
   Either NOT_SCHEDULED or the callback id.  */
static int readchar_callback = NOT_SCHEDULED;

static int readchar(void);
static void reset_readchar(void);
static void reschedule(void);
static int read_prim (void *buf, int count);
static int write_prim (const void *buf, int count);

static int remote_is_stdio = 0;

remote_fildes_t remote_desc = INVALID_DESCRIPTOR;
remote_fildes_t listen_desc = INVALID_DESCRIPTOR;

int is_remote_connected(void) { return remote_desc != INVALID_DESCRIPTOR; }

/* Return true if the remote connection is over stdio.  */

int is_remote_connection_is_stdio(void) { return remote_is_stdio; }

static void enable_async_notification(int fd) {
#if defined(F_SETFL) && defined(FASYNC)
  int save_fcntl_flags;

  save_fcntl_flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, save_fcntl_flags | FASYNC);
#if defined(F_SETOWN)
  fcntl(fd, F_SETOWN, getpid());
#endif
#endif
}

static int handle_accept_event(int err, remote_client_data client_data) {
  struct sockaddr_in sockaddr;
  socklen_t tmp;

  tmp = sizeof(sockaddr);
  remote_desc = accept(listen_desc, (struct sockaddr *)&sockaddr, &tmp);
  if (remote_desc == -1) perror("Accept failed");

  /* Enable TCP keep alive process. */
  tmp = 1;
  setsockopt(remote_desc, SOL_SOCKET, SO_KEEPALIVE, (char *)&tmp, sizeof(tmp));

  /* Tell TCP not to delay small packets.  This greatly speeds up
     interactive response. */
  tmp = 1;
  setsockopt(remote_desc, IPPROTO_TCP, TCP_NODELAY, (char *)&tmp, sizeof(tmp));

  signal(SIGPIPE, SIG_IGN); /* If we don't do this, then server simply
                               exits when the remote side dies.  */
  /* Even if !RUN_ONCE no longer notice new connections.  Still keep the
     descriptor open for add_file_handler to wait for a new connection.  */
  delete_file_handler(listen_desc);

  /* Convert IP address to string.  */
  fprintf(stderr, "Remote Connect from host %s\n",
          inet_ntoa(sockaddr.sin_addr));

  enable_async_notification(remote_desc);

  /* Register the event loop handler.  */
  add_file_handler(remote_desc, handle_serial_event, NULL);

#if 0
  /* We have a new connection now.  If we were disconnected
     tracing, there's a window where the target could report a stop
     event to the event loop, and since we have a connection now, we'd
     try to send vStopped notifications to GDB.  But, don't do that
     until GDB as selected all-stop/non-stop, and has queried the
     threads' status ('?').  */
  target_async(0);
#endif

  return 0;
}

/* Prepare for a later connection to a remote debugger.
   NAME is the filename used for communication.  */
void remote_prepare(char *name) {
  char *port_str;
  int port;
  struct sockaddr_in sockaddr;
  socklen_t tmp;
  char *port_end;

  remote_is_stdio = 0;
  if (strcmp(name, STDIO_CONNECTION_NAME) == 0) {
    /* We need to record fact that we're using stdio sooner than the
       call to remote_open so start_inferior knows the connection is
       via stdio.  */
    remote_is_stdio = 1;
    return;
  }

  port_str = strchr(name, ':');
  if (port_str == NULL) {
    return;
  }

  port = strtoul(port_str + 1, &port_end, 10);
  if (port_str[1] == '\0' || *port_end != '\0')
    printf("Bad port argument: %s", name);

  listen_desc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_desc == -1) perror("Can't open socket");

  /* Allow rapid reuse of this port. */
  tmp = 1;
  setsockopt(listen_desc, SOL_SOCKET, SO_REUSEADDR, (char *)&tmp, sizeof(tmp));

  sockaddr.sin_family = PF_INET;
  sockaddr.sin_port = htons(port);
  sockaddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(listen_desc, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) ||
      listen(listen_desc, 1))
    perror("Can't bind address");
}

/* Open a connection to a remote debugger.
   NAME is the filename used for communication.  */

void remote_open(char *name) {
  char *port_str;

  port_str = strchr(name, ':');

  if (strcmp(name, STDIO_CONNECTION_NAME) == 0) {
    fprintf(stderr, "Remote debugging using stdio\n");

    /* Use stdin as the handle of the connection.
       We only select on reads, for example.  */
    remote_desc = fileno(stdin);

    enable_async_notification(remote_desc);

    /* Register the event loop handler.  */
    add_file_handler(remote_desc, handle_serial_event, NULL);
  }
  else if (port_str == NULL) {
    struct stat statbuf;

    if (stat(name, &statbuf) == 0 &&
        (S_ISCHR(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode)))
      remote_desc = open(name, O_RDWR);
    else {
      errno = EINVAL;
      remote_desc = -1;
    }

    if (remote_desc < 0) perror("Could not open remote device");

#ifdef HAVE_TERMIOS
    {
      struct termios termios;
      tcgetattr(remote_desc, &termios);

      termios.c_iflag = 0;
      termios.c_oflag = 0;
      termios.c_lflag = 0;
      termios.c_cflag &= ~(CSIZE | PARENB);
      termios.c_cflag |= CLOCAL | CS8;
      termios.c_cc[VMIN] = 1;
      termios.c_cc[VTIME] = 0;

      tcsetattr(remote_desc, TCSANOW, &termios);
    }
#endif

#ifdef HAVE_TERMIO
    {
      struct termio termio;
      ioctl(remote_desc, TCGETA, &termio);

      termio.c_iflag = 0;
      termio.c_oflag = 0;
      termio.c_lflag = 0;
      termio.c_cflag &= ~(CSIZE | PARENB);
      termio.c_cflag |= CLOCAL | CS8;
      termio.c_cc[VMIN] = 1;
      termio.c_cc[VTIME] = 0;

      ioctl(remote_desc, TCSETA, &termio);
    }
#endif

#ifdef HAVE_SGTTY
    {
      struct sgttyb sg;

      ioctl(remote_desc, TIOCGETP, &sg);
      sg.sg_flags = RAW;
      ioctl(remote_desc, TIOCSETP, &sg);
    }
#endif

    fprintf(stderr, "Remote debugging using %s\n", name);

    enable_async_notification(remote_desc);

    /* Register the event loop handler.  */
    add_file_handler(remote_desc, handle_serial_event, NULL);
  }
  else {
    int port;
    socklen_t len;
    struct sockaddr_in sockaddr;

    len = sizeof(sockaddr);
    if (getsockname(listen_desc, (struct sockaddr *)&sockaddr, &len) < 0 ||
        len < sizeof(sockaddr))
      perror("Can't determine port");
    port = ntohs(sockaddr.sin_port);

    fprintf(stderr, "Listening on port %d\n", port);
    fflush(stderr);

    /* Register the event loop handler.  */
    add_file_handler(listen_desc, handle_accept_event, NULL);
  }
}

void remote_close(void) {
  delete_file_handler(remote_desc);

  if (!is_remote_connection_is_stdio()) close(remote_desc);

  remote_desc = INVALID_DESCRIPTOR;

  reset_readchar();
}

/* Come here when we get an input interrupt from the remote side.  This
   interrupt should only be active while we are waiting for the child to do
   something.  Thus this assumes readchar:bufcnt is 0.
   About the only thing that should come through is a ^C, which
   will cause us to request child interruption.  */

static void input_interrupt(int unused) {
  fd_set readset;
  struct timeval immediate = {0, 0};

  /* Protect against spurious interrupts.  This has been observed to
     be a problem under NetBSD 1.4 and 1.5.  */

  FD_ZERO(&readset);
  FD_SET(remote_desc, &readset);

  if (select(remote_desc + 1, &readset, 0, 0, &immediate) > 0) {
    int cc;
    char c = 0;

    cc = read_prim(&c, 1);

    if (cc != 1 || c != '\003') {
      fprintf(stderr, "input_interrupt, count = %d c = %d ('%c')\n", cc, c, c);
      return;
    }

    //(*the_target->request_interrupt) ();
  }
}

/* Check if the remote side sent us an interrupt request (^C).  */
void check_remote_input_interrupt_request(void) {
  /* This function may be called before establishing communications,
     therefore we need to validate the remote descriptor.  */

  if (remote_desc == INVALID_DESCRIPTOR) return;

  input_interrupt(0);
}

/* Asynchronous I/O support.  SIGIO must be enabled when waiting, in order to
   accept Control-C from the client, and must be disabled when talking to
   the client.  */

static void unblock_async_io(void) {
  sigset_t sigio_set;

  sigemptyset(&sigio_set);
  sigaddset(&sigio_set, SIGIO);
  sigprocmask(SIG_UNBLOCK, &sigio_set, NULL);
}

/* Current state of asynchronous I/O.  */
static int async_io_enabled;

/* Enable asynchronous I/O.  */
void enable_async_io(void) {
  if (async_io_enabled) return;

  signal(SIGIO, input_interrupt);
  async_io_enabled = 1;
}

/* Disable asynchronous I/O.  */
void disable_async_io(void) {
  if (!async_io_enabled) return;

  signal(SIGIO, SIG_IGN);
  async_io_enabled = 0;
}

void initialize_async_io(void) {
  /* Make sure that async I/O starts disabled.  */
  async_io_enabled = 1;
  disable_async_io();

  /* Make sure the signal is unblocked.  */
  unblock_async_io();
}

/* Internal buffer used by readchar.
   These are global to readchar because reschedule_remote needs to be
   able to tell whether the buffer is empty.  */

static unsigned char readchar_buf[BUFSIZ];
static int readchar_bufcnt = 0;
static unsigned char *readchar_bufp;

/* Returns next char from remote .  -1 if error.  */

static int readchar(void) {
  int ch;

  if (readchar_bufcnt == 0) {
    readchar_bufcnt = read_prim(readchar_buf, sizeof(readchar_buf));

    if (readchar_bufcnt <= 0) {
      if (readchar_bufcnt == 0)
        fprintf(stderr, "readchar: Got EOF\n");
      else
        perror("readchar");

      return -1;
    }

    readchar_bufp = readchar_buf;
  }

  readchar_bufcnt--;
  ch = *readchar_bufp++;
  reschedule();
  return ch;
}

/* Reset the readchar state machine.  */

static void reset_readchar(void) {
  readchar_bufcnt = 0;
  if (readchar_callback != NOT_SCHEDULED) {
    delete_callback_event(readchar_callback);
    readchar_callback = NOT_SCHEDULED;
  }
}

/* Process remaining data in readchar_buf.  */
static int process_remaining(void *context) {
  int res;

  /* This is a one-shot event.  */
  readchar_callback = NOT_SCHEDULED;

  if (readchar_bufcnt > 0)
    res = handle_serial_event(0, NULL);
  else
    res = 0;

  return res;
}

static void reschedule(void) {
  if (readchar_bufcnt > 0 && readchar_callback == NOT_SCHEDULED)
    readchar_callback = append_callback_event(process_remaining, NULL);
}

static int
write_prim (const void *buf, int count)
{
  if (is_remote_connection_is_stdio ())
    return write (fileno (stdout), buf, count);
  else
    return write (remote_desc, buf, count);
}

/* Read COUNT bytes from the client and store in BUF.
   The result is the number of bytes read or -1 if error.
   This may return less than COUNT.  */

static int
read_prim (void *buf, int count)
{
  if (is_remote_connection_is_stdio ())
    return read (fileno (stdin), buf, count);
  else
    return read (remote_desc, buf, count);
}

